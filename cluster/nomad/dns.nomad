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
job "dnstuff" {
  datacenters = ["dc1"]
  type = "service"
  spread {
    attribute = "${node.unique.id}"    
  }
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
    count = 2

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
    task "advertise" {
      driver = "docker"
      env {
        ROUTER_ID="10.243.123.${NOMAD_ALLOC_INDEX}"
        HELLO_INTERVAL="2"
        VIP_ADVERTISE="10.88.99.33"
      }
      config {
        image = "docker.svcs.alexroth.me/anycaster:latest"
        cap_add = ["net_admin", "net_broadcast", "net_raw"]
        volumes = ["new/hc.sh:/healthcheck.sh", "/run/consul/consul.sock:/consul.sock" ]
      }
      template {
        data = <<EOH
#!/bin/bash

set -euxo pipefail

if ! dig +time=3 +tries=3 +short @127.0.0.1 google.com; then
	echo "Can't reach local. Internet down?"
	if dig +time=3 +tries=3 +short @1.1.1.1 facebook.com; then
		echo "internet up, DNS down. Failing HC"
                exit 1
	else
		if dig +time=5 +tries=3 +short @8.8.4.4 amazon.com; then
                        echo "google up, cloudflare down, DNS down. Failing HC"
                        exit 1
                fi
        fi
                
else
	exit 0
fi
  EOH
        destination = "new/hc.sh"
        perms = "777"       
      }

      resources {
        cpu    = 10 # 500 MHz
        memory = 500 # 256MB
      }
    }
  }
}
