job "traefik" {
  region      = "global"
  datacenters = ["dc1"]
  type        = "service"


   constraint {
     attribute = "${node.class}"
     value     = "primary"
   }

  group "traefik" {
    count = 1

    network {
      mode = "bridge"
      port "http" {
        to = 80
      }
      port "https" {
        to = 443
      }
    }

      service {
        name = "traefik"
        port = "http"
        task = "proxy"
        check {
          name     = "alive"
          type     = "tcp"
          port     = "http"
          interval = "10s"
          timeout  = "2s"
        }
        check {
          name     = "alive"
          type     = "tcp"
          port     = "https"
          interval = "10s"
          timeout  = "2s"
        }

        connect {
          native = true
        }
      }

    task "advertise" {
      driver = "docker"
      env {
        ROUTER_ID="10.243.121.${NOMAD_ALLOC_INDEX}"
        HELLO_INTERVAL="2"
        VIP_ADVERTISE="10.88.99.39"
      }
      config {
        image = "docker.svcs.alexroth.me/anycaster:1.0.1"
        cap_add = ["net_admin", "net_broadcast", "net_raw"]
        volumes = ["new/hc.sh:/healthcheck.sh", "/run/consul/consul.sock:/consul.sock" ]
      }
      template {
        data = <<EOH
#!/bin/bash

set -euxo pipefail
curl -f -q http://127.0.0.1:80/ping
EOH
        destination = "new/hc.sh"
        perms = "777"       
      }

      resources {
        cpu    = 10 # 500 MHz
        memory = 200 # 256MB
      }
    }


    task "proxy" {
      vault {
          policies = ["traefik-policy"]
      }

      driver = "docker"
      env {
        GCE_SERVICE_ACCOUNT_FILE = "/etc/traefik/svcaccount.json"
      }
      config {
        image        = "traefik:v2.6.1"

        volumes = [
          "secrets/traefik.yml:/etc/traefik/traefik.yml",
          "secrets/traefik_dynamic.yml:/etc/traefik/dynamic/config.yml",
          "secrets/svcaccount.json:/etc/traefik/svcaccount.json",
          "secrets/ca.crt:/etc/traefik/services-ca.crt",
          "/dockershare/traefik/:/state/",
        ]
      }

      template {
        destination = "secrets/svcaccount.json"
        data = "{{with secret \"secret/data/dns-svcaccount\"}}{{.Data.data.json}}{{end}}"
      }
      template {
        destination = "secrets/traefik.yml"
        data = <<EOF
entryPoints:
  web:
    address: ":80"
  websecure:
    address: ":443"
ping:
    entryPoint: "web"
api:
    dashboard: true
log:
    level: "DEBUG"
certificatesResolvers:
  myresolver:
    acme:
      email: "alex@alexroth.me"
      storage: "/state/acme.json"
      dnsChallenge:
        provider: gcloud
        resolvers:
          - '1.1.1.1:53'
          - '1.0.0.1:53'
providers:
  file:
    directory: "/etc/traefik/dynamic"
  consulCatalog:
    prefix: "traefik"
    connectAware: true
    connectByDefault: true
    exposedByDefault: false
    endpoint: 
      address: "{{ env "meta.hostsvcaddr" }}:8500"
      scheme: "http"
      token: "{{with secret "consul/creds/traefik-role"}}{{.Data.token}}{{end}}"
EOF
      }
      template {
        destination = "secrets/traefik_dynamic.yml"
        data = <<EOF
tls:
  options:
    withClientCert:
      clientAuth:
        caFiles:
          - '/etc/traefik/services-ca.crt'
        clientAuthType: RequireAndVerifyClientCert
http:
  routers:
    http_catchall:
      rule: "hostregexp(`{host:.+}`)"
      service: "noop@internal"
      entryPoints:
        - web
      middlewares:
        - redirect_to_https
    api:
      rule: "Host(`traefik.home.svcs.alexroth.me`)"
      entryPoints:
        - websecure
      service: "api@internal"
      tls: {}
    wildcard_certs:
      rule: "Host(`wildcardnotreal.home.svcs.alexroth.me`)"
      service: "noop@internal"
      tls:
        certResolver: myresolver
        domains:
          - main: '*.home.svcs.alexroth.me'
  middlewares:
    redirect_to_https:
      redirectScheme:
        scheme: https 
EOF
      }


      template {
        destination = "secrets/ca.crt"
        data = <<EOF
-----BEGIN CERTIFICATE-----
MIIDSDCCAjCgAwIBAgIUOohpMZ1azrnU3RqfQUN0yyP5agQwDQYJKoZIhvcNAQEL
BQAwFTETMBEGA1UEAwwKU2VydmljZXNDQTAeFw0yMDA5MTQyMjU0MzBaFw0zMDA5
MTIyMjU0MzBaMBUxEzARBgNVBAMMClNlcnZpY2VzQ0EwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQC40uaOWHPJJsM8yIbfnJ3cxpWqhWX219FbdGPNYZuc
16OpmHAIgDRo5XxDAp1eOmqV31iNQ7ns2NZpRU0zydiyod0Ba/Iyl7YhEQpEsdlM
HIPirV5qQdYjN8dvvNXIiMwJ7qRxY2dEhkk+xs4GVZRzDHyPIWqnDPM1vypZYdpR
NXrlbfrEAymkea56BtXlMBpNDlFQad2ALoPNutUre/AMm9UQHi05Nbm3DmNANOrL
R19Gkd+ZJeVrk1We06zWRqZM4vcHT+gIIvrQ79rwACMTXMKZjjI8bfEZgySxnUoS
RzMSScuSjGdbzq4h9jewifbuJ0eJlT7fYm6HfEIJeXWVAgMBAAGjgY8wgYwwHQYD
VR0OBBYEFCHf6bjpihPsmU7dKATMwgbKDMeRMFAGA1UdIwRJMEeAFCHf6bjpihPs
mU7dKATMwgbKDMeRoRmkFzAVMRMwEQYDVQQDDApTZXJ2aWNlc0NBghQ6iGkxnVrO
udTdGp9BQ3TLI/lqBDAMBgNVHRMEBTADAQH/MAsGA1UdDwQEAwIBBjANBgkqhkiG
9w0BAQsFAAOCAQEAYUMzRGLi7nDx51upb488lg04U8oftVRm8eyG5OTD6K/1S2ui
QsNB7Waaf1eE2lZQvQhdvppfw3maVoU4WQM5PoqmgXhCRDW9KtKyr4C+eYCylZKT
Z/yUFxOxRM88fNq31iNx7FsNwr7MwoWYv5PmQfnbrUsV4rpkqan5lLCs2cQhPRTM
wWoDHhgi2JvXE/fowttc3X5p4K8yw9K4/PUbk5Rhj3Ux6LJ0+WQWQ7iezLajyLct
5g5cXIpOjdAZ63FC3xCOx0aFYpic2JF5o9tqvAZbZx0XRpkr9f7/B646S0J8IUXf
6kC15zzhSTkMoGYFo/gkmxJVgDUO9xINgHangg==
-----END CERTIFICATE----- 
EOF
      }


      resources {
        cpu    = 100
        memory = 256
      }
    }
  }
}
