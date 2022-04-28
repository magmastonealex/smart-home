output "db_name" {
    value = postgresql_database.app.name
}

output "db_username" {
    value = postgresql_role.app.name
}

output "cred_path" {
    value = vault_generic_secret.secret.path
}

output "backup_cred_path" {
    value = vault_generic_secret.backup_secret.path
}

output "policy" {
    value = vault_policy.app_policy.name
}

output "backup_policy" {
    value = vault_policy.app_backup_policy.name
}