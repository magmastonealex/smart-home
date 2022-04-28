#!/bin/bash

echo '------------------------------ Starting patroni ------------------------------'
HOME=$HOME USER=$USER /usr/bin/python3 /usr/bin/patroni /patroni.yml 2>&1
echo '------------------------------ Patroni finished ------------------------------'
