job "conduit" {
  datacenters = ["dc1"]

   constraint {
     attribute = "${node.class}"
     value     = "primary"
   } 

  group "conduit" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "conduit"
      port = "6167"

      connect {
        sidecar_service {}
      }

      tags = [
        "traefik.enable=true",
        "traefik.consulcatalog.connect=true",
        "traefik.http.routers.conduit.rule=Host(`conduit.home.svcs.alexroth.me`)",
        "traefik.http.routers.conduit.tls=true",
      ]
    }

    task "conduit" {
      driver = "docker"


      config {
        image = "matrixconduit/matrix-conduit:latest-commit-faa0cdb5"
        volumes = [
          "/dockershare/conduit/db:/var/lib/matrix-conduit/conduit_db",
          "/dockershare/conduit/conduit.toml:/srv/conduit/conduit.toml",
          "/dockershare/conduit/resolv.conf:/etc/resolv.conf:ro",
        ]
      }

      resources {
        cpu    = 500
        memory = 1024
      }
    }
  }
}
