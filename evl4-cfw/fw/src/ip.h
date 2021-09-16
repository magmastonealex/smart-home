#pragma once
#include <stdint.h>

typedef struct {
    uint32_t addr;
    uint32_t nmask;
    uint32_t gw;
} net_ip_info;

net_ip_info *ip_get_ip_info();
