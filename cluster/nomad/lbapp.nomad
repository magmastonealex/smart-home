job "demo-webapp" {
  datacenters = ["dc1"]

  group "demo" {
    count = 1
    spread {
      attribute = "${node.unique.id}"
    }
    network {
      port  "http"{
        to = -1
        host_network = "public"
      }
    }

    service {
      name = "demo-webapp"
      port = "http"

      tags = [
        "traefik.enable=true",
        "traefik.http.routers.http.rule=Path(`/myapp`)",
      ]

      check {
        type     = "http"
        path     = "/"
        interval = "2s"
        timeout  = "2s"
      }
    }

    task "server" {
    vault {
        policies = ["test-policy-temp"]
    }
template {
  data = <<EOH
# Lines starting with a # are ignored

# Empty lines are also ignored
PORT = {{ env "NOMAD_PORT_http" }}
NODE_IP = {{ env "NOMAD_IP_http" }}
API_KEY="{{with secret "secret/data/hellotest"}}{{.Data.data.bladibla}}{{end}}"
EOH

  destination = "secrets/file.env"
  env         = true
}

      driver = "docker"
      config {
        image = "hashicorp/demo-webapp-lb-guide"
        ports = ["http"]
      }
    }
  }
}
