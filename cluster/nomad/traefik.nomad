job "traefik" {
  region      = "global"
  datacenters = ["dc1"]
  type        = "service"

  group "traefik" {
    count = 1

    network {
      mode = "bridge"
      port "http" {
        to = 8080
        host_network="default"
      }

      port "api" {
        to = 8081
        host_network="default"
      }
    }

      service {
        name = "traefik"
        port = "http"
        task = "proxy"
        check {
          name     = "alive"
          type     = "tcp"
          port     = "http"
          interval = "10s"
          timeout  = "2s"
        }

        connect {
          native = true
        }
      }

    task "advertise" {
      driver = "docker"
      env {
        ROUTER_ID="10.243.121.${NOMAD_ALLOC_INDEX}"
        HELLO_INTERVAL="2"
        VIP_ADVERTISE="10.88.99.39"
      }
      config {
        image = "docker.svcs.alexroth.me/anycaster:1.0.1"
        cap_add = ["net_admin", "net_broadcast", "net_raw"]
        volumes = ["new/hc.sh:/healthcheck.sh", "/run/consul/consul.sock:/consul.sock" ]
      }
      template {
        data = <<EOH
#!/bin/bash

set -euxo pipefail
curl -f -q http://127.0.0.1:8080/ping

EOH
        destination = "new/hc.sh"
        perms = "777"       
      }

      resources {
        cpu    = 10 # 500 MHz
        memory = 200 # 256MB
      }
    }


    task "proxy" {
      vault {
          policies = ["traefik-policy"]
      }

      driver = "docker"

      config {
        image        = "traefik:v2.6.1"

        volumes = [
          "secrets/traefik.toml:/etc/traefik/traefik.toml",
        ]
      }

      template {
        data = <<EOF
[entryPoints]
    [entryPoints.http]
    address = ":8080"
    [entryPoints.traefik]
    address = ":8081"
[ping]
    entryPoint = "http"
[api]
    dashboard = true
    insecure  = true
[log]
    level = "DEBUG"
# Enable Consul Catalog configuration backend.
[providers.consulCatalog]
    prefix           = "traefik"
    connectAware     = true
    connectByDefault = true
    exposedByDefault = false
    [providers.consulCatalog.endpoint]
      address = "{{ env "meta.hostsvcaddr" }}:8500"
      scheme  = "http"
      token = "{{with secret "secret/data/traefik_token"}}{{.Data.data.token}}{{end}}"
EOF

        destination = "secrets/traefik.toml"
      }

      resources {
        cpu    = 100
        memory = 256
      }
    }
  }
}
