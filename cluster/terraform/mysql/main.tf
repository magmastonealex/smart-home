terraform {
  required_providers {
    mysql = {
      source  = "winebarrel/mysql"
      version = "~> 1.10.2"
    }
  }
  required_version = ">= 0.13"
}

variable "MYSQL_ROOT_PASSWORD" {
  type = string
}

variable "MYSQL_ENDPOINT" {
  type = string
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

provider "mysql" {
  endpoint = "${var.MYSQL_ENDPOINT}"
  username = "root"
  password = "${var.MYSQL_ROOT_PASSWORD}"
}