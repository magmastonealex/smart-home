
all: consultls consul_secrets.yml

consultls:
	make -C setup/consul/consul-ca all

createsecrets:
	echo "---" > setup/consul/consul_secrets.yml
	echo "consultokens:" >> setup/consul/consul_secrets.yml

consultokens_%:
	ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(shell uuidgen -r)" --name "  $(patsubst consultokens_%,%,$@)" >> setup/consul/consul_secrets.yml

consul_secrets.yml: createsecrets consultokens_thor consultokens_zeus consultokens_mouse consultokens_nomad_thor consultokens_nomad_zeus consultokens_nomad_mouse

createvaultsecrets:
	echo "---" > setup/nomad/vault_secrets.yml
	echo "vault_tokens:" >> setup/nomad/vault_secrets.yml

vaultsecret_%:
	ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(shell vault token create -policy nomad-server -period 768h -orphan | head -n 3 | tail -n 1 | awk '{print $$2}')" --name "  $(patsubst vaultsecret_%,%,$@)" >> setup/nomad/vault_secrets.yml

vault_secrets.yml: createvaultsecrets vaultsecret_thor vaultsecret_zeus vaultsecret_mouse


createvaultsecrets_consul:
	echo "---" > setup/consul/vault_secrets_consul.yml
	echo "vault_tokens:" >> setup/consul/vault_secrets_consul.yml

vaultsecret_consul_%:
	ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(shell vault token create -policy consul-server -period 768h -orphan | head -n 3 | tail -n 1 | awk '{print $$2}')" --name "  $(patsubst vaultsecret_consul_%,%,$@)" >> setup/consul/vault_secrets_consul.yml

vault_secrets_consul.yml: createvaultsecrets_consul vaultsecret_consul_thor vaultsecret_consul_zeus vaultsecret_consul_mouse

clean:
	make -C setup/consul/consul-ca $@
	rm -f setup/consul/consul_secrets.yml
	rm -f setup/nomad/vault_secrets.yml
	rm -f setup/consul/vault_secrets_consul.yml
	rm -f setup/nomad/nomad_secrets.yml
	rm -f unseal_secrets.yml

#ansible-playbook -i inventory install_setup_nomad.yml --vault-id vault_pass.txt

.PHONY: all clean install consultls createsecrets consultokens_%
