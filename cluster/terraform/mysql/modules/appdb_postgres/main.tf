

terraform {
  required_providers {
    postgresql = {
      source = "cyrilgdn/postgresql"
      version = "1.15.0"
    }
  }
}

resource "postgresql_database" "app" {
  name = var.app_name
  owner = postgresql_role.app.name
}

resource "postgresql_role" "app" {
  name     = "${var.app_name}_user"
  login    = true
  password = random_password.app_user_password.result
}

resource "random_password" "app_user_password" {
  length           = 32
  special          = var.allow_special_in_password
  override_special = "!#$@*&^~-_"
}

resource "vault_generic_secret" "secret" {
  path = "secret/${var.app_name}_pg_creds"

  data_json = jsonencode({
      user  = postgresql_role.app.name
      password = random_password.app_user_password.result
  })
}

# Policy for application
resource "vault_policy" "app_policy" {
  name = "${var.app_name}-policy"

  policy = <<EOT
path "secret/data/${var.app_name}_pg_creds" {
  capabilities = ["read"]
}
EOT
}

