
resource "consul_config_entry" "count-api-tester" {
  kind = "service-intentions"
  name = "count-api"

  config_json = jsonencode({
    Sources = [
      {
        Precedence = 9
        Type = "consul"
        Name   = "count-dashboard"
        Action = "allow"
      }
    ]
  })
}

resource "consul_config_entry" "traefik-dashboard-tester" {
  kind = "service-intentions"
  name = "count-dashboard"

  config_json = jsonencode({
    Sources = [
      {
        Precedence = 9
        Type = "consul"
        Name   = "traefik"
        Action = "allow"
      }
    ]
  })
}

resource "consul_config_entry" "whoami-traefik" {
  kind = "service-intentions"
  name = "whoami"

  config_json = jsonencode({
    Sources = [
      {
        Name   = "traefik"
        Action = "allow"
      }
    ]
  })
}
