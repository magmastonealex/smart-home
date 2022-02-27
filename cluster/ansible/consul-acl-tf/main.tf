# Configure the Consul provider - assumes SSH port forwarding
provider "consul" {
 address="127.0.0.1:8500"
}

variable "nodes" {
   type = set(string)
   default = ["vm1", "vm2", "vm3"] 
}

