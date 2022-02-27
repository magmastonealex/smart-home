#! /bin/bash
set -euxo pipefail

echo "---" > unseal_secrets.yml

read -p 'First Secret' SECRET
ansible-vault encrypt_string --vault-password-file vault_pass.txt "$SECRET" --name "vault_unseal_1" >> unseal_secrets.yml
read -p 'Second Secret' SECRET
ansible-vault encrypt_string --vault-password-file vault_pass.txt "$SECRET" --name "vault_unseal_2" >> unseal_secrets.yml
read -p 'Third Secret' SECRET
ansible-vault encrypt_string --vault-password-file vault_pass.txt "$SECRET" --name "vault_unseal_3" >> unseal_secrets.yml
