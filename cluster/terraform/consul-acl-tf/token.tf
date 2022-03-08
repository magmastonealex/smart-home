resource "consul_acl_token" "anonymous-token" {
  description = "Anonymous Token"
  policies = ["${consul_acl_policy.consul-anonymous-policy.name}"]
}

resource "consul_acl_token" "agent-tokens" {
  description = "${each.key}-consul-access-token"
  for_each = var.nodes
  policies = ["${consul_acl_policy.consul-host-policy[each.key].name}"]
}

resource "consul_acl_token" "nomad-tokens" {
  description = "${each.key}-nomad-access-token"
  for_each = var.nodes
  policies = ["${consul_acl_policy.consul-nomad-policy.name}"]
}

