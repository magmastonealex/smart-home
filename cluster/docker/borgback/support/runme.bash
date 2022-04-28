#! /bin/bash

set -euo pipefail

if [[ ! -f /config.json ]]; then
	echo "Must have /config.json"
	exit 1
fi

if [[ ! -f /id_rsa ]]; then
	echo "Must have /id_rsa"
	exit 1
fi

if [[ ! -f /known_hosts ]]; then
	echo "Must have /known_hosts"
	exit 1
fi

export BORG_RSH="ssh -o UserKnownHostsFile=/known_hosts -i /id_rsa"

if [[ ! -d /backup ]]; then
	echo "Must have backup mounted at /backup"
	exit 1
fi

BACKUP_HOST=$(cat /config.json | jq -r .host)
BACKUP_PATH=$(cat /config.json | jq -r .path)
export BORG_PASSPHRASE=$(cat /config.json | jq -r .passphrase)

if [[ -z "$BACKUP_HOST" ]]; then
	echo "Must define host in config"
	exit 1
fi

if [[ -z "$BACKUP_PATH" ]]; then
	echo "Must define path in config"
	exit 1
fi

if [[ -z "$BORG_PASSPHRASE" ]]; then
	echo "Must define passphras in config"
	exit 1
fi


borg create -v --stats --progress --compression lzma,5 "borguser@$BACKUP_HOST:$BACKUP_PATH::cluster-$(date +%Y-%m-%d)-$(date +%s)" /backup
borg prune --list --keep-daily 14 --keep-weekly 6 --keep-monthly 12 borguser@$BACKUP_HOST:$BACKUP_PATH

