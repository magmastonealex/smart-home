job "unbound" {
  datacenters = ["dc1"]
  type = "service"
  spread {
    attribute = "${node.unique.id}"    
  }

  # Use affinity instead of constraint - want to fall back to secondary if need be.
  affinity {
     attribute = "${node.class}"
     value     = "primary"
  }

  group "cache" {
    count = 2

    network {
      mode = "bridge"
    }

    spread {
      attribute = "${node.unique.id}"
    }

    task "unbound" {
      driver = "docker"

      config {
        image = "docker.svcs.alexroth.me/unbound:1.0.1"

        volumes = [
            "local/:/etc/unbound/",
        ]
      }

      template {
        data = <<EOH
. IN DS 20326 8 2 E06D44B80B8F1D39A95C0B0D7C65D08458E880409BBC683457104237C7F8EC8D
  EOH
        destination = "local/root.key"
        perms = "666"     
      }

      artifact {
        source      = "https://www.internic.net/domain/named.root"
        destination = "local/root.hints.tmp"
        mode = "file"
      }

      template {
          data = file("blacklist.conf")
          destination = "local/blacklist.conf"
          perms = "666"
      }

      template {
        source      = "local/root.hints.tmp"
        destination = "local/root.hints"
        perms = "666"
      }

      template {
          perms = "666"
          destination = "local/unbound.conf"
          data = <<EOH
server:
    directory: /etc/unbound
    logfile: ""
    verbosity: 1
    edns-buffer-size: 1232

    interface: 10.88.99.5{{ env "NOMAD_ALLOC_INDEX" }}
    port: 53

    do-ip4: yes

    do-ip6: no
    do-udp: yes
    do-tcp: yes

    access-control: 10.0.0.0/8 allow
    access-control: 127.0.0.0/8 allow

    root-hints: "/etc/unbound/root.hints"

    hide-identity: yes
    hide-version: yes

    harden-glue: yes

    harden-dnssec-stripped: yes

    use-caps-for-id: yes

    cache-min-ttl: 120
    cache-max-ttl: 86400

    prefetch: yes
    prefetch-key: yes
    num-threads: 4

    msg-cache-slabs: 8
    rrset-cache-slabs: 8
    infra-cache-slabs: 8
    key-cache-slabs: 8

    rrset-cache-size: 256m
    msg-cache-size: 128m
    so-rcvbuf: 1m
    private-address: 192.168.0.0/16
    private-address: 172.16.0.0/12
    private-address: 10.0.0.0/8
    private-domain: "alexroth.me"
    unwanted-reply-threshold: 10000

    do-not-query-localhost: no
    auto-trust-anchor-file: "/etc/unbound/root.key"
    val-clean-additional: yes
    include: "/etc/unbound/blacklist.conf"
EOH
      }

      resources {
        cpu    = 200
        memory = 768
      }
    }
    task "advertise" {
      driver = "docker"
      env {
        ROUTER_ID="10.243.123.${NOMAD_ALLOC_INDEX}"
        HELLO_INTERVAL="2"
        VIP_ADVERTISE="10.88.99.5${NOMAD_ALLOC_INDEX}"
      }
      config {
        image = "docker.svcs.alexroth.me/anycaster:1.0.1"
        cap_add = ["net_admin", "net_broadcast", "net_raw"]
        volumes = ["new/hc.sh:/healthcheck.sh"]
      }
      template {
        data = <<EOH
#!/bin/bash

set -euxo pipefail

if ! dig +time=3 +tries=3 +short @10.88.99.5{{ env "NOMAD_ALLOC_INDEX" }} google.com; then
	echo "Can't reach local. Internet down?"
	if dig +time=3 +tries=3 +short @1.1.1.1 facebook.com; then
		echo "internet up, DNS down. Failing HC"
                exit 1
	else
		if dig +time=5 +tries=3 +short @8.8.4.4 amazon.com; then
                        echo "google up, cloudflare down, DNS down. Failing HC"
                        exit 1
                fi
        fi
                
else
	exit 0
fi
  EOH
        destination = "new/hc.sh"
        perms = "777"       
      }

      resources {
        cpu    = 10 # 500 MHz
        memory = 500 # 256MB
      }
    }
  }
}
