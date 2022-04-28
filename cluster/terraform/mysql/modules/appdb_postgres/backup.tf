# Sets up backups for the application DB using a periodic Nomad job.


resource "postgresql_role" "backup_app" {
  name     = "backup_${var.app_name}_user"
  login    = true
  password = random_password.backup_user_password.result
}

resource "postgresql_default_privileges" "backup_tables_default" {
  role     = postgresql_role.backup_app.name
  database = postgresql_database.app.name
  schema   = "public"

  owner       = postgresql_role.app.name
  object_type = "table"
  privileges  = ["SELECT"]
}

resource "postgresql_default_privileges" "backup_sequence_default" {
  role     = postgresql_role.backup_app.name
  database = postgresql_database.app.name
  schema   = "public"

  owner       = postgresql_role.app.name
  object_type = "sequence"
  privileges  = ["SELECT"]
}

resource "postgresql_grant" "backup_connect" {
  role     = postgresql_role.backup_app.name
  database = postgresql_database.app.name
  schema   = "public"
  object_type = "database"
  privileges  = ["CONNECT"]
}

resource "postgresql_grant" "backup_select_table" {
  role     = postgresql_role.backup_app.name
  database = postgresql_database.app.name
  schema   = "public"
  object_type = "table"
  objects     = []
  privileges  = ["SELECT"]
}
resource "postgresql_grant" "backup_select_sequence" {
  role     = postgresql_role.backup_app.name
  database = postgresql_database.app.name
  schema   = "public"
  object_type = "sequence"
  objects     = []
  privileges  = ["SELECT"]
}


resource "vault_generic_secret" "backup_secret" {
  path = "secret/backup_${var.app_name}_pg_creds"

  data_json = jsonencode({
      user  = postgresql_role.backup_app.name
      password = random_password.backup_user_password.result
  })
}

resource "vault_policy" "app_backup_policy" {
  name = "${var.app_name}-backup-policy"

  policy = <<EOT
path "secret/data/backup_${var.app_name}_pg_creds" {
  capabilities = ["read"]
}
EOT
}

resource "random_password" "backup_user_password" {
  length           = 32
  special          = true
  override_special = "!#$@*&^~-_"
}