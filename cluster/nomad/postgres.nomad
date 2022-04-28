job "postgres" {
  region      = "global"
  datacenters = ["dc1"]
  type        = "service"
  spread {
    attribute = "${node.unique.id}"    
  }

  constraint {
     attribute = "${node.class}"
     value     = "primary"
  }

  group "postgres-access" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "postgres-input"
      port = "5432"
    }

    task "socat" {

      template {
        data = <<EOH
{{ range service "master.postdb" }}
POSTGRES_HOST={{.Address}}
POSTGRES_PORT={{.Port}}
{{end}}
EOH

        destination = "secrets/file.env"
        env         = true
    }

      driver = "docker"

      config {
        image = "alpine/socat"
        args  = ["tcp-listen:5432,fork,reuseaddr", "tcp-connect:${POSTGRES_HOST}:${POSTGRES_PORT}"]
      }

      resources {
        cpu    = 50
        memory = 50
      }
    }
    task "advertise" {
      driver = "docker"
      env {
        ROUTER_ID="20.231.231.${NOMAD_ALLOC_INDEX}"
        HELLO_INTERVAL="2"
        VIP_ADVERTISE="10.88.99.20"
      }
      config {
        image = "docker.svcs.alexroth.me/anycaster:1.0.1"
        cap_add = ["net_admin", "net_broadcast", "net_raw"]
        volumes = ["new/hc.sh:/healthcheck.sh"]
      }
      template {
        data = <<EOH
#!/bin/bash

exit 0
EOH
        destination = "new/hc.sh"
        perms = "777"       
      }

      resources {
        cpu    = 10
        memory = 200
      }
    }

  }


  group "postgres" {
    count = 2

    network {
      mode = "bridge"
      port "postgres" {
        to = 5432
      }
      port "patroni" {
        to = 8008
      }
    }


    task "proxy" {
      vault {
          policies = ["patroni-policy"]
      }

      driver = "docker"
      config {
        image = "docker.svcs.alexroth.me/postgres-patroni:1.0.4"

        volumes = [
          "secrets/patroni.yml:/patroni.yml",
          "/pg_data:/pg_data",
        ]
      }

      template {
        destination = "secrets/patroni.yml"
        data = <<EOF
name: "{{ env "NOMAD_ALLOC_ID" }}"
scope: postdb
namespace: /hapostgres/
consul:
  url: http://{{ env "meta.hostsvcaddr" }}:8500
  register_service: true
  token: {{with secret "secret/data/postgres"}}{{.Data.data.consul_token}}{{end}}
postgresql:
  connect_address: "{{ env "NOMAD_IP_postgres"}}:{{ env "NOMAD_HOST_PORT_postgres" }}"
  bin_dir: /usr/lib/postgresql/13/bin
  data_dir: /pg_data/data
  listen: "*:5432"
  authentication:
    replication:
      username: replicator
      password: "{{with secret "secret/data/postgres"}}{{.Data.data.replication_password}}{{end}}"
    superuser:
      username: postgres
      password: "{{with secret "secret/data/postgres"}}{{.Data.data.superuser_password}}{{end}}"

restapi:
  connect_address: "{{ env "NOMAD_IP_patroni"}}:{{ env "NOMAD_HOST_PORT_patroni" }}"
  listen: "0.0.0.0:8008"
  authentication:
    username: patroni
    password: "{{with secret "secret/data/postgres"}}{{.Data.data.patroni_password}}{{end}}"
bootstrap:
  dcs:
    ttl: 30
    loop_wait: 10
    retry_timeout: 10
    maximum_lag_on_failover: 1048576
  pg_hba:
    - local all all  md5
    - host all all 127.0.0.1/32 md5
    - host all all ::1/128 md5
    - host all all ::1/128 md5
    - host all all 0.0.0.0/0 md5
    - host replication replicator 10.20.105.10/32 md5
    - host replication replicator 10.20.105.11/32 md5
  initdb:
    - encoding: UTF8
EOF
      }

      resources {
        cpu    = 1000
        memory = 1024
      }
    }
  }
}
