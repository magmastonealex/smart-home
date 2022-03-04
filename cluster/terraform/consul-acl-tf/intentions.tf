
resource "consul_config_entry" "count-api-tester" {
  kind = "service-intentions"
  name = "count-api"

  config_json = jsonencode({
    Sources = [
      {
        Name   = "count-dashboard"
        Action = "allow"
      }
    ]
  })
}
