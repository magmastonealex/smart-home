terraform {
  required_providers {
    mysql = {
      source  = "winebarrel/mysql"
      version = "~> 1.10.2"
    }
  }
  required_version = ">= 0.13"
}

resource "mysql_database" "app" {
  name = var.app_name
}

resource "mysql_user" "app" {
  user               = "${var.app_name}_user"
  host               = "127.0.0.1"
  plaintext_password = random_password.app_user_password.result
}

# Grant the main application r/w permissions
resource "mysql_grant" "app" {
  user       = mysql_user.app.user
  host       = mysql_user.app.host
  database   = mysql_database.app.name
  privileges = ["ALL PRIVILEGES"]
}

resource "random_password" "app_user_password" {
  length           = 32
  special          = true
  override_special = "!#$@*&^~-_"
}

resource "vault_generic_secret" "secret" {
  path = "secret/${var.app_name}_creds"

  data_json = jsonencode({
      user  = mysql_user.app.user
      password = random_password.app_user_password.result
  })
}

# Policy for application
resource "vault_policy" "app_policy" {
  name = "${var.app_name}-policy"

  policy = <<EOT
path "secret/data/${var.app_name}_creds" {
  capabilities = ["read"]
}
EOT
}

