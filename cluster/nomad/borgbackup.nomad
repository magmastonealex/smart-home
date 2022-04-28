job "borg_backup" {
  periodic {
    cron             = "0 15 0 * * * *"
    prohibit_overlap = true
  }
  datacenters = ["dc1"]
  type        = "batch"
  group "backup" {

    network {
      mode = "bridge"
    }

    task "backup" {

      vault {
          policies = ["borg-backup-policy"]
      }

      driver = "docker"

      template {
        destination = "secrets/config.json"
        data = <<EOF
{{with secret "secret/data/borg"}}
  {
      "host": "{{.Data.data.host}}",
      "path": "{{.Data.data.path}}",
      "passphrase": "{{.Data.data.passphrase}}"
  }
{{end}}
EOF
      }

      template {
        destination = "secrets/id_rsa"
        perms = "600"
        data = "{{with secret \"secret/data/borg\"}}{{.Data.data.key}}\n{{end}}"
      }

      template {
        destination = "secrets/known_hosts"
        data = "{{with secret \"secret/data/borg\"}}{{.Data.data.hostkey}}\n{{end}}"
      }

      config {
        image = "docker.svcs.alexroth.me/borg-backup:1.0.5"
        
        volumes = [
          "secrets/config.json:/config.json",
          "secrets/id_rsa:/id_rsa",
          "secrets/known_hosts:/known_hosts",
          "/dockershare/mysqlback:/backup/mysql:ro",
          "/dockershare/conduit:/backup/conduit:ro",
          "/dockershare/bw-data:/backup/bw-data:ro",
          "/dockershare/mealie:/backup/mealie:ro",
          "/dockershare/gitea:/backup/gitea:ro",
        ]
      }

      resources {
        cpu    = 150
        memory = 512
      }
    }
  }
}