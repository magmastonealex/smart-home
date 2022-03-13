# Sets up backups for the application DB using a periodic Nomad job.

resource "mysql_user" "backup_app" {
  user               = "backup_${var.app_name}_user"
  host               = "127.0.0.1"
  plaintext_password = random_password.backup_user_password.result
}

resource "mysql_grant" "backup_app" {
  user       = mysql_user.backup_app.user
  host       = mysql_user.backup_app.host
  database   = mysql_database.app.name
  privileges = ["SELECT", "LOCK TABLES", "TRIGGER", "SHOW VIEW"]
}

resource "vault_generic_secret" "backup_secret" {
  path = "secret/backup_${var.app_name}_creds"

  data_json = jsonencode({
      user  = mysql_user.backup_app.user
      password = random_password.backup_user_password.result
  })
}

resource "vault_policy" "app_backup_policy" {
  name = "${var.app_name}-backup-policy"

  policy = <<EOT
path "secret/data/backup_${var.app_name}_creds" {
  capabilities = ["read"]
}
EOT
}

resource "random_password" "backup_user_password" {
  length           = 32
  special          = true
  override_special = "!#$@*&^~-_"
}

#${var.app_name}_backup
#${vault_policy.app_backup_policy.name}


#docker.svcs.alexroth.me/mysql-backup:1.0.0