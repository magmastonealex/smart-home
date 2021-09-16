#include "ip.h"
#include "netcommon.h"

net_ip_info info = {IPADDR_FROM_OCTETS(192, 168, 50, 30), IPADDR_FROM_OCTETS(255,255,255,255), IPADDR_FROM_OCTETS(192, 168, 50, 1)};

net_ip_info *ip_get_ip_info() {
    return &info;
}
