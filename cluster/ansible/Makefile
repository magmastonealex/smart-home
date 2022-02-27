
all: consul_secrets.yml consultls

consultls:
	make -C consul-ca all

createsecrets:
	echo "---" > consul_secrets.yml
	echo "consultokens:" >> consul_secrets.yml

consultokens_%:
	ansible-vault encrypt_string --vault-password-file vault_pass.txt "$(shell uuidgen -r)" --name "  $(patsubst consultokens_%,%,$@)" >> consul_secrets.yml

consul_secrets.yml: createsecrets consultokens_vm1 consultokens_vm2 consultokens_vm3 consultokens_nomad_vm1 consultokens_nomad_vm2 consultokens_nomad_vm3

clean:
	make -C consul-ca $@

#ansible-playbook -i inventory install_setup_nomad.yml --vault-id vault_pass.txt

.PHONY: all clean install consultls createsecrets consultokens_%
