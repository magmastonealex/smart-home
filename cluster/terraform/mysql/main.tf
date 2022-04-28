terraform {
  required_providers {
    postgresql = {
      source = "cyrilgdn/postgresql"
      version = "1.15.0"
    }
  }
  
  required_version = ">= 0.13"
}

# Configure the Consul provider - assumes SSH port forwarding
provider "consul" {
 address="127.0.0.1:8500"
}

provider "nomad" {
  address = "http://zeus.svcs.alexroth.me:4646"
}

provider "vault" {
}

variable "POSTGRES_SUPERUSER_PASSWORD" {
  type = string
}

provider "postgresql" {
  host            = "10.88.99.20"
  port            = 5432
  username        = "postgres"
  password        = var.POSTGRES_SUPERUSER_PASSWORD
  connect_timeout = 5
  sslmode = "disable"
}