# Deployment Steps

1. set hostnames in consul-ca/Makefile, set up inventory.
2. run make to provision TLS for Consul & generate Consul tokens.
3. Run consul keygen, then encrypt the token with `ansible-vault encrypt_string --vault-password-file a_password_file 'foobar' --name 'consulkey'`. Put that in install_setup_consul.
4. Provision the cluster. Run the ansible playbooks - base, consul.
5. Bootstrap Consul ACL system and put token in secrets - `ansible-vault encrypt_string --vault-password-file a_password_file 'foobar' --name 'consul_management' >> consul_secrets.yml`. Export token as CONSUL_HTTP_AUTH.
6. Apply consul_acls.yml playbook to provision all tokens.
7. Do terraform importing of the tokens. Start with anonymous: `terraform import consul_acl_token.anonymous-token 00000000-0000-0000-0000-000000000002`, then the VMs (using accessor ID): `terraform import 'consul_acl_token.agent-tokens["vm3"]' db2d...`  then Nomad tokens `terraform import 'consul_acl_token.nomad-tokens["vm3"]' db2d...`
7. Do a terraform apply to provision all the policies.
7. Restart consul on all the servers, check the logs to make sure everyone's finding eachother (run consul members too)
8. Run the ansible playbooks for gluster, bird, and nomad.
9. Grab your new Nomad token - nomad acl bootstrap (I don't have need for more restrictive tokens since I'm the only one doing things anyways)
9. Set NOMAD_AUTH accordingly, and apply an anonymous read-only policy: nomad acl policy apply -description "Anonymous policy (full-access)" anonymous anonymous_nomad.hcl
10. Deploy vault - run the tls and vault playbooks.
11. Set export VAULT_TLS_SERVER_NAME=vm3.... and VAULT_ADDR=... and run `vault status` to ensure things came up.
12. Initialize Vault in the usual way - vault operator init and friends.
13. Set your VAULT_TOKEN, and do a terraform apply in install_setup_nomad.yml to set up policies.
14. Run make vault_secrets.yml to generate vault tokens, and apply the nomad_vault playbook to configure Nomad to use Vault.
9. Enjoy your shiny new cluster.
