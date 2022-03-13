#! /bin/bash

export TF_VAR_MYSQL_ENDPOINT=$(curl -s -f localhost:8500/v1/catalog/service/mysql-admin | jq -r .[0].ServiceAddress):$(curl -s -f localhost:8500/v1/catalog/service/mysql-admin | jq -r .[0].ServicePort)
