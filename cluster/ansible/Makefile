
all:
	make -C consul-ca $@

clean:
	make -C consul-ca $@

install:
	ansible-playbook -i inventory install_setup_consul.yml --vault-id vault_pass.txt
	ansible-playbook -i inventory install_setup_nomad.yml --vault-id vault_pass.txt

.PHONY: all clean install
