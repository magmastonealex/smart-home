
variable "applications" {
    default = ["hello"]
    type = set(string)
}

module "appdb_mysql" {
  source = "./modules/appdb"
  for_each = var.applications
  app_name     = each.key
}

resource "consul_config_entry" "mysql-admin" {
  kind = "service-intentions"
  name = "mysql"

  config_json = jsonencode({
    Sources = concat([
      {
        Name   = "mysql-admin"
        Action = "allow"
        Precedence = 9
        Type = "consul"
      }
    ],
    [for s in var.applications : {Name = s, Action = "allow", Precedence = 9, Type = "consul"}],
    [for s in var.applications : {Name = "${s}-backup", Action = "allow", Precedence = 9, Type = "consul"}])
  })
}


resource "nomad_job" "backup_jobs" {
    jobspec = data.template_file.jobspec[each.key].rendered
    for_each = var.applications
    hcl2 {
        enabled = true
    }
}

data "template_file" "jobspec" {
  template = "${file("${path.module}/backupjobspec.hcl")}"
  for_each = var.applications
  vars = {
    app_name = each.key
  }
}