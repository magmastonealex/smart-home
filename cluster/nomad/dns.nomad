# There can only be a single job definition per file. This job is named
# "example" so it will create a job with the ID and Name "example".

# The "job" stanza is the top-most configuration option in the job
# specification. A job is a declarative specification of tasks that Nomad
# should run. Jobs have a globally unique name, one or many task groups, which
# are themselves collections of one or many tasks.
#
# For more information and examples on the "job" stanza, please see
# the online documentation at:
#
#     https://www.nomadproject.io/docs/job-specification/job
#
job "dns" {
  datacenters = ["dc1"]
  type = "service"

   constraint {
     attribute = "${node.class}"
     value     = "primary"
   }

  update {
    max_parallel = 1
    min_healthy_time = "10s"
    healthy_deadline = "3m"
    progress_deadline = "10m"
    auto_revert = false
    canary = 0
  }

  migrate {
    max_parallel = 1
    health_check = "checks"
    min_healthy_time = "10s"
    healthy_deadline = "5m"
  }

  group "cache" {
    count = 1

    network {
      mode = "bridge"
      port "dns" {
        to = 53
        host_network = "public"
      }
      port "web" {
        to = 80
        host_network = "public"
      }
    }

    service {
      name = "dnsserver"
      port = "dns"

      # The "check" stanza instructs Nomad to create a Consul health check for
      # this service. A sample check is provided here for your convenience;
      # uncomment it to enable it. The "check" stanza is documented in the
      # "service" stanza documentation.

      # check {
      #   name     = "alive"
      #   type     = "tcp"
      #   interval = "10s"
      #   timeout  = "2s"
      # }

    }
    service {
      name = "dnsserveradmin"
      port = "web"

      # The "check" stanza instructs Nomad to create a Consul health check for
      # this service. A sample check is provided here for your convenience;
      # uncomment it to enable it. The "check" stanza is documented in the
      # "service" stanza documentation.

      # check {
      #   name     = "alive"
      #   type     = "tcp"
      #   interval = "10s"
      #   timeout  = "2s"
      # }

    }

    restart {
      attempts = 2
      interval = "30m"

      delay = "15s"

      mode = "fail"
    }

    ephemeral_disk {
      size = 300
    }

    spread {
      attribute = "${node.unique.id}"
    }

    task "pihole" {
      driver = "docker"
      env {
        TZ="America/Toronto"
      }
      config {
        image = "pihole/pihole:2022.02.1"

        ports = ["dns","web"]

        mount {
          type = "bind"
          target = "/etc/pihole"
          source = "/dockershare/pihole_${NOMAD_ALLOC_INDEX}"
          readonly = false
        }
      }

      resources {
        cpu    = 500 # 500 MHz
        memory = 256 # 256MB
      }

    }
  }
}
