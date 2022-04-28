job "mealie" {
  datacenters = ["dc1"]
  priority = 10
   constraint {
     attribute = "${node.class}"
     value     = "primary"
   } 

  group "mealie" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "mealie"
      port = "3000"
      task = "mealie-frontend"

      connect {
        sidecar_service {
        }
      }

      tags = [
        "traefik.enable=true",
        "traefik.consulcatalog.connect=true",
        "traefik.http.routers.mealie.rule=Host(`mealie.home.svcs.alexroth.me`)",
        "traefik.http.routers.mealie.tls=true",
      ]
    }
    task "mealie-frontend" {
      driver = "docker"

      env {
        API_URL = "http://127.0.0.1:9000"
      }

      config {
        image = "hkotel/mealie:frontend-nightly"
        volumes = [
          "/dockershare/mealie:/app/data",
        ]
      }

      resources {
        cpu    = 200
        memory = 200
      }
    }

    task "mealie-api" {
      driver = "docker"

      vault {
          policies = ["mealie-policy"]
      }

      template {
        data = <<EOH
PUID=1000
PGID=1000
TZ=America/Toronto
ALLOW_SIGNUP=true
{{with secret "secret/data/mealie_pg_creds"}}
DB_ENGINE=postgres
POSTGRES_USER={{.Data.data.user}}
POSTGRES_PASSWORD={{.Data.data.password}}
POSTGRES_SERVER=10.88.99.20
POSTGRES_PORT=5432
POSTGRES_DB=mealie
{{end}}
EOH
        destination = "secrets/file.env"
        env         = true
    }

      config {
        image = "hkotel/mealie:api-nightly"
        volumes = [
          "/dockershare/mealie:/app/data",
        ]
      }

      resources {
        cpu    = 200
        memory = 500
      }
    }
  }
}
