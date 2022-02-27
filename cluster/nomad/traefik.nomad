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
        host_network="public"
      }

      port "api" {
        to = 8081
        host_network="public"
      }
    }

    service {
      name = "traefik"
      port = "http"
      check {
        name     = "alive"
        type     = "tcp"
        port     = "http"
        interval = "10s"
        timeout  = "2s"
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


    task "traefik" {
      driver = "docker"

      config {
        image        = "traefik:v2.2"

        volumes = [
          "local/traefik.toml:/etc/traefik/traefik.toml",
          "/run/consul/consul.sock:/consul.sock"
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

# Enable Consul Catalog configuration backend.
[providers.consulCatalog]
    prefix           = "traefik"
    exposedByDefault = false

    [providers.consulCatalog.endpoint]
      address = "{{ env "meta.hostsvcaddr" }}:8500"
      scheme  = "http"
EOF

        destination = "local/traefik.toml"
      }

      resources {
        cpu    = 100
        memory = 256
      }
    }
  }
}
