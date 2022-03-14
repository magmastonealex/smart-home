job "gitea" {
  datacenters = ["dc1"]

   constraint {
     attribute = "${node.class}"
     value     = "primary"
   } 

   group "gitea-ssh-frontend" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "gitea-ssh-frontend"
      port = "22"

      connect {
        sidecar_service {
            proxy {
                upstreams {
                    destination_name = "gitea-ssh"
                    local_bind_port = 2202
                }
            }
        }
      }
    }

    task "socat" {
      driver = "docker"

      config {
        image = "alpine/socat"
        args  = ["tcp-listen:22,fork,reuseaddr", "tcp-connect:127.0.0.1:2202"]
      }

      resources {
        cpu    = 50
        memory = 50
      }
    }
    task "advertise" {
      driver = "docker"
      env {
        ROUTER_ID="19.241.131.${NOMAD_ALLOC_INDEX}"
        HELLO_INTERVAL="2"
        VIP_ADVERTISE="10.88.99.40"
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

  group "gitea" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "gitea"
      port = "3000"

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
        "traefik.http.routers.gitea.rule=Host(`gitea.home.svcs.alexroth.me`)",
        "traefik.http.routers.gitea.tls=true",
      ]
    }

    service {
      name = "gitea-ssh"
      port = "22"

      connect {
        sidecar_service {
        }
      }
    }

    task "gitea" {
      driver = "docker"

      vault {
          policies = ["gitea-policy"]
      }

      config {
        image = "gitea/gitea:1.15.4"
        volumes = [
          "/dockershare/gitea/:/data",
          "/etc/timezone:/etc/timezone:ro",
          "/etc/localtime:/etc/localtime:ro",
        ]
      }

      template {
        data = <<EOH
USER_UID="1000"
USER_GID="1000"
{{with secret "secret/data/gitea_creds"}}
GITEA__database__PASSWD="{{.Data.data.password}}"
GITEA__database__USER="{{.Data.data.user}}"
{{end}}
GITEA__database__NAME="gitea"
GITEA__database__HOST="127.0.0.1:3306"
GITEA__database__DB_TYPE="mysql"
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
