provider "vault" {
}
resource "vault_policy" "consul-server" {
  name = "consul-server"

  policy = <<EOT
path  "/sys/mounts" {
  capabilities = [ "read" ]
}

path "/sys/mounts/connect_root" {
  capabilities = [ "create", "read", "update", "delete", "list" ]
}
path "/sys/mounts/connect_inter" {
  capabilities = [ "create", "read", "update", "delete", "list" ]
}
path "/connect_root/*" {
  capabilities = [ "create", "read", "update", "delete", "list" ]
}
path "/connect_inter/*" { 
  capabilities = [ "create", "read", "update", "delete", "list" ]
}

path "auth/token/renew-self" {
  capabilities = [ "update" ]
}
path "auth/token/lookup-self" {
  capabilities = [ "read" ]
}
EOT
}

resource "vault_policy" "nomad-server" {
  name = "nomad-server"

  policy = <<EOT
# Allow creating tokens under "nomad-cluster" token role. The token role name
# should be updated if "nomad-cluster" is not used.
path "auth/token/create/nomad-cluster" {
  capabilities = ["update"]
}

# Allow looking up "nomad-cluster" token role. The token role name should be
# updated if "nomad-cluster" is not used.
path "auth/token/roles/nomad-cluster" {
  capabilities = ["read"]
}

# Allow looking up the token passed to Nomad to validate # the token has the
# proper capabilities. This is provided by the "default" policy.
path "auth/token/lookup-self" {
  capabilities = ["read"]
}

# Allow looking up incoming tokens to validate they have permissions to access
# the tokens they are requesting. This is only required if
# `allow_unauthenticated` is set to false.
path "auth/token/lookup" {
  capabilities = ["update"]
}

# Allow revoking tokens that should no longer exist. This allows revoking
# tokens for dead tasks.
path "auth/token/revoke-accessor" {
  capabilities = ["update"]
}

# Allow checking the capabilities of our own token. This is used to validate the
# token upon startup.
path "sys/capabilities-self" {
  capabilities = ["update"]
}

# Allow our own token to be renewed.
path "auth/token/renew-self" {
  capabilities = ["update"]
}
EOT
}


resource "vault_token_auth_backend_role" "nomad-cluster" {
  role_name              = "nomad-cluster"
  disallowed_policies    = ["nomad-server"]
  orphan                 = true
  token_period           = "259200"
  renewable              = true
}


resource "vault_mount" "kv2-generic-secrets" {
  path        = "secret"
  type        = "kv-v2"
  description = "generic secrets"
}

resource "vault_policy" "test-policy-temp" {
  name = "test-policy-temp"

  policy = <<EOT
# Allow creating tokens under "nomad-cluster" token role. The token role name
# should be updated if "nomad-cluster" is not used.
path "secret/data/hellotest" {
  capabilities = ["read"]
}
EOT
}

resource "vault_policy" "traefik-policy" {
  name = "traefik-policy"

  policy = <<EOT
path "secret/data/traefik_token" {
  capabilities = ["read"]
}
EOT
}
