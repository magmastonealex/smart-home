#! /bin/bash
set -euxo pipefail

mkdir -p /run/bird

echo "Configuring with ID: $ROUTER_ID Interval: $HELLO_INTERVAL VIP: $VIP_ADVERTISE"

function makegood() {
cat bird.conf.tmpl | sed "s/ROUTER_ID/$ROUTER_ID/" | sed "s/HELLO_INTERVAL/$HELLO_INTERVAL/" > /etc/bird.conf
}
function makebad() {
cat bird.conf.tmpl | sed "s/ROUTER_ID/$ROUTER_ID/" | sed "s/HELLO_INTERVAL/$HELLO_INTERVAL/" | sed 's/export all; # REPLACE ME/export none;/' > /etc/bird.conf
}

set +e
ip addr add $VIP_ADVERTISE/32 dev lo
rc=$?
if [[ $rc -eq 0 ]] || [[ $rc -eq 2 ]]; then
  echo "Added $VIP_ADVERTISE"
else
  echo "ip addr failed - permissions?"
  exit 1
fi
set -e

makebad
# fork bird off as a daemon
bird -d -c /etc/bird.conf &


while true; do
	echo "Running health check..."
        if /healthcheck.sh; then
            echo "Healthcheck passed - advertising IP"
            makegood
            birdc configure
        else
            echo "Healthcheck failure - withdrawing route"
            makebad
            birdc configure
        fi
        sleep 10
done
