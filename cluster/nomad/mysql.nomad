job "mysql" {
  datacenters = ["dc1"]

  constraint {
    attribute = "${node.class}"
    value     = "primary"
  } 

  group "mysql" {
    count = 1

    network {
      mode = "bridge"
    }

    service {
      name = "mysql"
      port = "3306"

      connect {
        sidecar_service {}
      }
    }

    task "mysql" {
      driver = "docker"

      env = {
        MYSQL_ONETIME_PASSWORD = "true"
        MYSQL_RANDOM_ROOT_PASSWORD = "true"
      }

      config {
        image = "mysql:8-debian"
        volumes = [
          "/dockershare/mysql/:/var/lib/mysql",
        ]
      }

      resources {
        cpu    = 1500
        memory = 1024
      }
    }
  }
}
