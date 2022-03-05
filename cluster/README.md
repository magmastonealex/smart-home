Dumping ground for now. Contains Ansible scripts used to bootstrap a micro-PC cluster with Nomad, Consul, Vault, Gluster, and BGP, providing HA services for my homelab.

TODOs:
- Configure Traefik for HTTPS
- Configure Wireguard between cluster hosts and cloud VPS
- Configure Gluster to run over Wireguard
- Configure Traefik to use Consul Service Mesh - done, but broken within Vagrant.
- Configure anycast BGP peering with cloud VPS

