job "${app_name}_mysql_backup" {
  periodic {
    cron             = "0 1 0 * * * *"
    prohibit_overlap = true
  }
  datacenters = ["dc1"]
  type        = "batch"
  group "backup" {

    network {
      mode = "bridge"
    }

    service {
      name = "${app_name}-backup"

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
    task "backup" {

      vault {
          policies = ["${app_name}-backup-policy"]
      }

      driver = "docker"

      template {
        destination = "secrets/config.json"
        data = <<EOF
{{with secret "secret/data/backup_${app_name}_creds"}}
  {
      "host": "127.0.0.1",
      "port": 3306,
      "user": "{{.Data.data.user}}",
      "password": "{{.Data.data.password}}",
      "db": "${app_name}"
  }
{{end}}
EOF
      }

      config {
        image = "docker.svcs.alexroth.me/mysql-backup:1.0.4"
        
        volumes = [
          "secrets/config.json:/config.json",
          "/dockershare/mysqlback/${app_name}:/backup",
        ]
      }

      resources {
        cpu    = 150
        memory = 512
      }
    }
  }
}