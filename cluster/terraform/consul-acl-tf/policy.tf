resource "consul_acl_policy" "consul-host-policy" {
  name        = "${each.key}-consul-access"
  for_each = var.nodes
  rules       = <<-RULE
    node "${each.key}" {
      policy = "write"
    }

    agent "${each.key}" {
      policy = "write"
    }

    node_prefix "" {
      policy = "read"
    }

    key "" {
      policy = "read"
    }

    service_prefix "" {
      policy = "read"
    }
   RULE
}

resource "consul_acl_policy" "consul-nomad-policy" {
  name = "nomad-consul-access"
  rules = <<-RULE
service_prefix "" {
  policy = "write"
}
key_prefix "" {
  policy = "write"
}
agent_prefix "" {
  policy = "read"
}
event_prefix "" {
  policy = "write"
}
query_prefix "" {
  policy = "write"
}
session_prefix "" {
  policy = "write"
}
node_prefix "" {
  policy = "read"
}
acl = "write"
RULE
}

resource "consul_acl_policy" "consul-traefik-policy" {
  name = "traefik-consul-access"
  rules = <<-RULE
key_prefix "traefik" {
  policy = "write"
}

service "traefik" {
  policy = "write"
}

agent_prefix "" {
  policy = "read"
}

node_prefix "" {
  policy = "read"
}

service_prefix "" {
  policy = "read"
}
RULE
}

resource "consul_acl_policy" "consul-anonymous-policy" {
  name   = "anonymous-consul-access"
  rules  = <<-RULE
service_prefix "" {
  policy = "read"
}
key_prefix "" {
  policy = "read"
}
node_prefix "" {
  policy = "read"
}
RULE
}

resource "consul_acl_policy" "patroni-policy" {
  name = "patroni-access-policy"
  rules = <<-RULE
service_prefix "postdb" {
    policy = "write"
}
key_prefix "/hapostgres/postdb" {
    policy = "write"
}
key_prefix "hapostgres/postdb" {
    policy = "write"
}
session_prefix "" {
    policy = "write"
}
RULE
}

