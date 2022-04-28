job "whoami" {
  datacenters = ["dc1"]

   constraint {
     attribute = "${node.class}"
     value     = "primary"
   } 

  group "whoami" {
    count = 2

    network {
      mode = "bridge"
    }

    service {
      name = "whoami"
      port = "8020"

      connect {
        sidecar_service {}
      }

      tags = [
        "traefik.enable=true",
        "traefik.consulcatalog.connect=true",
        "traefik.http.routers.whoami.rule=Host(`whoami.home.svcs.alexroth.me`)",
        "traefik.http.routers.whoami.tls=true"
      ]
    }

    task "whoami" {
      driver = "docker"

      config {
        image = "traefik/whoami"
        ports = ["web"]
        args  = ["--port", "8020"]
      }

      resources {
        cpu    = 100
        memory = 128
      }
    }
  }
}
