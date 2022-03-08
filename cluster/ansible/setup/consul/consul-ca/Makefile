all: dc1-server-consul-zeus.pem dc1-server-consul-thor.pem dc1-server-consul-mouse.pem

consul-agent-ca.pem:
	consul tls ca create

dc1-server-consul-%.pem: consul-agent-ca.pem
	consul tls cert create -server -dc dc1 -domain consul
	mv -v dc1-server-consul-0.pem $@
	mv -v dc1-server-consul-0-key.pem $(patsubst %.pem,%-key.pem,$@)



clean:
	rm -f *.pem

.PHONY: all clean

