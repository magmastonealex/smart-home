Tiny Docker container to run BIRD, to support anycasting.

OSPF will be active on the $OSPF\_INTERFACE interface
Addresses on $ADVERTISE\_INTERFACE will be advertised over OSPF if /healthcheck.sh is returning a 0 code.

$HELLO\_INTERVAL is respected.

Based on the result of /healthcheck.sh,
