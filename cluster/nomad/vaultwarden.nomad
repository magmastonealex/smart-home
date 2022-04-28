job "vaultwarden" {
  datacenters = ["dc1"]

   constraint {
     attribute = "${node.class}"
     value     = "primary"
   } 

  group "vaultwarden" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "vaultwarden"
      port = "80"

      connect {
        sidecar_service {
            proxy {
                upstreams {
                    destination_name = "mysql"
                    local_bind_port = 3306
                }
            }
        }
      }

      tags = [
        "traefik.enable=true",
        "traefik.consulcatalog.connect=true",
        "traefik.http.routers.vaultwarden.rule=Host(`bitwarden.home.svcs.alexroth.me`)",
        "traefik.http.routers.vaultwarden.tls=true",
      ]
    }

    service {
      name = "vaultwardenws"
      port = "3012"

      connect {
        sidecar_service {
        }
      }

      tags = [
        "traefik.enable=true",
        "traefik.consulcatalog.connect=true",
        "traefik.http.routers.vaultwardenws.rule=Host(`bitwarden.home.svcs.alexroth.me`) && Path(`/notifications/hub`)",
        "traefik.http.routers.vaultwardenws.tls=true",
      ]
    }


    task "vaultwarden" {
      driver = "docker"

      vault {
          policies = ["passwords-policy"]
      }

      config {
        image = "vaultwarden/server:1.24.0"
        volumes = [
          "/dockershare/bw-data/:/data",
          "/etc/localtime:/etc/localtime:ro",
        ]
      }

      template {
        data = <<EOH
DOMAIN="https://bitwarden.home.svcs.alexroth.me"
{{with secret "secret/data/passwords_pg_creds"}}
DATABASE_URL=postgres://{{.Data.data.user}}:{{.Data.data.password}}@10.88.99.20/passwords
{{end}}
EOH
        destination = "secrets/file.env"
        env         = true
    }

      resources {
        cpu    = 500
        memory = 1024
      }
    }
  }
}
