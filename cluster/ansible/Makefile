
all: consul_secrets.yml consultls

consultls:
	make -C consul-ca all

createsecrets:
	echo "---" > consul_secrets.yml
	echo "consultokens:" >> consul_secrets.yml

consultokens_%:
	ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(shell uuidgen -r)" --name "  $(patsubst consultokens_%,%,$@)" >> consul_secrets.yml

consul_secrets.yml: createsecrets consultokens_vm1 consultokens_vm2 consultokens_vm3 consultokens_nomad_vm1 consultokens_nomad_vm2 consultokens_nomad_vm3

createvaultsecrets:
	echo "---" > vault_secrets.yml
	echo "vault_tokens:" >> vault_secrets.yml

vaultsecret_%:
	ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(shell vault token create -policy nomad-server -period 72h -orphan | head -n 3 | tail -n 1 | awk '{print $$2}')" --name "  $(patsubst vaultsecret_%,%,$@)" >> vault_secrets.yml

vault_secrets.yml: createvaultsecrets vaultsecret_vm1 vaultsecret_vm2 vaultsecret_vm3

clean:
	make -C consul-ca $@

#ansible-playbook -i inventory install_setup_nomad.yml --vault-id vault_pass.txt

.PHONY: all clean install consultls createsecrets consultokens_%
