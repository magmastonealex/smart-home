job "mysql-admin" {
  datacenters = ["dc1"]

  constraint {
    attribute = "${node.class}"
    value     = "primary"
  } 

  group "mysql-admin" {
    count = 1

    network {
      mode = "bridge"
      port "proxy" {
          to = 33306
      }
    }

    service {
      name = "mysql-admin"
      port = "proxy"

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
    }

    task "mysql-admin" {
      driver = "docker"

      config {
        image = "alpine/socat"
        ports = ["proxy"]
        args  = ["tcp-listen:33306,fork,reuseaddr", "tcp-connect:127.0.0.1:3306"]
      }

      resources {
        cpu    = 150
        memory = 256
      }
    }
  }
}
