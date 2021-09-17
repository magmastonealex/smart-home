#pragma once
#include "netcommon.h"

// Receive an ICMP packet.
// Most of these we don't do much with other than log,
// but we can respond to pings!
void icmp_recv(sk_buff *buf);
