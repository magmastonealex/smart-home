variable "postgres_applications" {
    default = ["gitdb", "passwords", "mealie", "freshrss"]
    type = set(string)
}

module "appdb_postgres" {
  source = "./modules/appdb_postgres"
  for_each = var.postgres_applications
  allow_special_in_password = each.key == "gitdb"
  app_name     = each.key
}

resource "nomad_job" "pg_backup_jobs" {
    jobspec = data.template_file.pg_jobspec[each.key].rendered
    for_each = var.postgres_applications
    hcl2 {
        enabled = true
    }
}

data "template_file" "pg_jobspec" {
  template = "${file("${path.module}/pg_backupjobspec.hcl")}"
  for_each = var.postgres_applications
  vars = {
    app_name = each.key
  }
}
