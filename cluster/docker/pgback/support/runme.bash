#! /bin/bash

set -euo pipefail

if [[ ! -f /config.json ]]; then
	echo "Must have /config.json"
	exit 1
fi

if [[ ! -d /backup ]]; then
	echo "Must have backup mounted at /backup"
	exit 1
fi

MYSQL_HOST=$(cat /config.json | jq -r .host)
MYSQL_PORT=$(cat /config.json | jq -r .port)
MYSQL_USER=$(cat /config.json | jq -r .user)
PG_PASSWORD=$(cat /config.json | jq -r .password)
MYSQL_DB=$(cat /config.json | jq -r .db)

if [[ -z "$MYSQL_HOST" ]]; then
	echo "Must define host in config"
	exit 1
fi

if [[ -z "$MYSQL_PORT" ]]; then
	echo "Must define port in config"
	exit 1
fi

if [[ -z "$MYSQL_USER" ]]; then
	echo "Must define user in config"
	exit 1
fi

if [[ -z "$PG_PASSWORD" ]]; then
	echo "Must define password in config"
	exit 1
fi

if [[ -z "$MYSQL_DB" ]]; then
	echo "Must define db in config"
	exit 1
fi

echo "*:*:*:*:$PG_PASSWORD" > ~/.pgpass
chmod 600 ~/.pgpass


pg_dump -U "$MYSQL_USER" -h "$MYSQL_HOST" -p "$MYSQL_PORT" "$MYSQL_DB" > /backup/dump-"$MYSQL_DB"-$(date +%s).sql

find /backup/dump-*.sql -mtime +7 -exec rm -v {} \;
