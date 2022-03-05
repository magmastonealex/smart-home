# Deployment Steps

*Note*: I set this up in Vagrant before deploying to a real cluster. That works great except for the NAT-ing Vagrant wants to do. I just manually deleted the default route and added my own via a shared `private_network` the rest of the cluster lived on. 

# Initial Keys & Setup

Get ready to provision the cluster.

1. Set up consul-ca/Makefile and Makefile with your hostnames (replacing vm1,vm2,vm3). I should make that dynamic. I have not bothered yet.
2. Run `make` to provision the Consul inter-server CA and generate Consul tokens.
3. Run keygen and encrypt the token with `ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(consul keygen)" --name "consulkey"`. Place this in `install_setup_consul`.
4. Update your Ansible inventory with connection details.
5. Confirm Ansible is working & start cluster setup - `ansible-playbook -i inventory 00_install_setup_base.yml --vault-id vault_pass.txt`

# Consul

1. Install & do initial Consul configuration - run `01_install_setup_consul.yml`. Confirm servers have found each other.
2. Set up port forwarding to Consul over ssh (`ssh -L 8500:localhost:8500 root@<vm>`)
3. Bootstrap Consul ACL system - `consul_acl_bootstrap`. Export this token as `CONSUL_HTTP_AUTH` and `TF_VAR_CONSUL_HTTP_AUTH`, and encrypt it: `ansible-vault encrypt_string --vault-password-file vault_pass.txt "$CONSUL_HTTP_AUTH" --name "consulkey" >> consul/consul_secrets.yml`
4. Run `01_consul_acls.yml` to create all your tokens.
5. Begin terraforming. In terraform/consul-acls-tf run `terraform init`, then import your tokens: `terraform import consul_acl_token.anonymous-token 00000000-0000-0000-0000-000000000002`, `terraform import 'consul_acl_token.agent-tokens["vm3"]' db2d...`, `terraform import 'consul_acl_token.nomad-tokens["vm3"]' db2d...`, etc for all of vm3, vm2, vm1.
6. Run terraform apply to set up all of the policies.
7. Restart consul on all the servers, and check logs looking for ACL errors. `consul members` should show you membership.

# Nomad

1. Run keygen and encrypt the token with `ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(nomad keygen)" --name "gossipkey" >> nomad/nomad_secrets.yml`
1. Install & setup Nomad - run `02_install_setup_nomad.yml` and check logs to ensure servers have joined one another.
1. Bootstrap the ACL subsystem - `nomad acl bootstrap`. Save this token as a `NOMAD_AUTH` env var.
1. Apply the read-only anonymous policy (if you want). `nomad acl policy apply -description "Anonymous Policy (full access)" anonymous support/anonymous_nomad.hcl`


# Vault

1. Run the `03_install_setup_tls.yml` playbook to set up TLS for all the servers
1. Run the `04_install_setup_vault.yml` playbook to set up Vault.
1. Export `VAULT_TLS_SERVER_NAME=vm3....` and `VAULT_ADDR=...` and run `vault status` to ensure vault came up okay.
1. Run `vault operator init`, and save everything you see in a safe place. Export `VAULT_TOKEN` with your root token.
1. Run `make_unseal_secrets.sh` and enter three key shares. This allows for the `unseal` playbook to work.
1. Run the `unseal.yml` playbook to unseal all the servers.
1. run `vault operator raft list-peers` to examine the peers list.
1. In `terraform/vault-acls-tf` run `terraform init` then `terraform apply` to configure all the policies. 
1. run `make vault_secrets.yml` and `make vault_secrets_consul.yml`.
1. Run the `05_install_setup_consul_vault.yml` and `05_install_setup_nomad_vault.yml` playbooks to connect Consul and Nomad to Vault.

# CSB #1: Bird and Gluster

If you're making use of shared storage or Anycasting, run the BIRD and Gluster playbooks accordingly.

# CSB #2: Ingress.

Deploy the Traefik job


