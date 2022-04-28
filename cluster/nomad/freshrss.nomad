job "freshrss" {
  datacenters = ["dc1"]
  priority = 10
   constraint {
     attribute = "${node.class}"
     value     = "primary"
   } 

  group "freshrss" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "freshrss"
      port = "80"

      connect {
        sidecar_service {
        }
      }

      tags = [
        "traefik.enable=true",
        "traefik.consulcatalog.connect=true",
        "traefik.http.routers.freshrss.rule=Host(`freshrss.home.svcs.alexroth.me`)",
        "traefik.http.routers.freshrss.tls=true",
      ]
    }
    task "freshrss" {
      driver = "docker"
      env {
          TZ = "America/Toronto"
          CRON_MIN = "*/20"
      }
      config {
        image = "freshrss/freshrss:1.19.2"
        volumes = [
          "/dockershare/freshrss/data:/var/www/FreshRSS/data",
          "/dockershare/freshrss/extensions:/var/www/FreshRSS/extensions"
        ]
      }

      resources {
        cpu    = 200
        memory = 200
      }
    }
  }
}
