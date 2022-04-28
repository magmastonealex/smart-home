job "${app_name}_pg_backup" {
  periodic {
    cron             = "0 2 0 * * * *"
    prohibit_overlap = true
  }
  datacenters = ["dc1"]
  type        = "batch"
  group "backup" {

    network {
      mode = "bridge"
    }

    service {
      name = "${app_name}-pg-backup"
    }
    task "backup" {

      vault {
          policies = ["${app_name}-backup-policy"]
      }

      driver = "docker"

      template {
        destination = "secrets/config.json"
        data = <<EOF
{{with secret "secret/data/backup_${app_name}_pg_creds"}}
  {
      "host": "10.88.99.20",
      "port": 5432,
      "user": "{{.Data.data.user}}",
      "password": "{{.Data.data.password}}",
      "db": "${app_name}"
  }
{{end}}
EOF
      }

      config {
        image = "docker.svcs.alexroth.me/pgback:1.0.8"
        
        volumes = [
          "secrets/config.json:/config.json",
          "/dockershare/mysqlback/${app_name}_pg:/backup",
        ]
      }

      resources {
        cpu    = 150
        memory = 512
      }
    }
  }
}