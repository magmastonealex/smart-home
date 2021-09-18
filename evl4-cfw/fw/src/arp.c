#include "arp.h"
#include "nic.h"
#include "ip.h"
#include <string.h>
#include "dbgserial.h"

typedef struct {
    uint8_t complete; // is entry complete?
    uint32_t ip;
    uint8_t hwaddr[6];
    uint8_t time; // Timeout - starts at 255 seconds.
    uint8_t use_time; // Counter for last use - reset to 255 every time it's used.
} arp_entry;

#define NUM_ARP_ENTRIES 10

arp_entry arp_entries[NUM_ARP_ENTRIES] = {0};

sk_buff arp_buff = {0};

#define ARP_OP_REQUEST htons(1)
#define ARP_OP_REPLY htons(2)

void arp_to(uint32_t src, uint32_t dst, uint8_t *addr);
void arp_for(uint32_t src, uint32_t addr);

void arp_debug_dump() {
    DBGprintf("ARP table dump: \n");
    for (uint8_t i = 0; i < NUM_ARP_ENTRIES; i++)
    {
        DBGprintf("\t time: %u, complete? %x", arp_entries[i].time, arp_entries[i].complete);
        DBGprintf(" MAC: ");
        print_mac(arp_entries[i].hwaddr);
        DBGprintf(" IP: ");
        print_ip(arp_entries[i].ip);
        DBGprintf("\n");
    }
    DBGprintf("ARP dump complete\n");
}


static uint8_t find_arp_slot() {
    // Look for the oldest entry, or ideally an expired one.
    uint8_t entry = 0;
    uint8_t oldest = 255;
    for (uint8_t i = 0; i < NUM_ARP_ENTRIES; i++)
    {
        if (arp_entries[i].time == 0) {
            // already expired - perfect!
            entry = i;
            break;
        }
        if (arp_entries[i].time < oldest) {
            oldest = arp_entries[i].time;
            entry = i;
        }
    }
    return entry;
}

void complete_arp(uint8_t *addr, uint32_t host) {
    for (uint8_t i = 0; i < NUM_ARP_ENTRIES; i++)
    {
        if (arp_entries[i].ip == host) {
            // found it!
            arp_entries[i].time = 255;
            arp_entries[i].complete = 1;
            memcpy(arp_entries[i].hwaddr, addr, 6);
            return;
        }    
    }
    
    
    // Didn't have outstanding request.

    uint8_t entry = find_arp_slot();

    arp_entries[entry].complete = 1;
    arp_entries[entry].ip = host;
    memcpy(arp_entries[entry].hwaddr, addr, 6);
    arp_entries[entry].time = 255;

}

void arp_in(sk_buff *buf) {
    if (buf->len < (sizeof(ether_hdr) + sizeof(arp_ether_ipv4))) {
        DBGprintf("[ARP] bad packet - too small\n");
        return;
    }
    arp_ether_ipv4 *arp = (arp_ether_ipv4*) (buf->buff + sizeof(ether_hdr));

    uint32_t my_addr = ip_get_ip_info()->addr;
    if (arp->htype == htons(ARP_HTYPE_ETHER) && arp->ptype == htons(ETH_P_IPV4)) {
        if (arp->tpa == my_addr) {
            // someone's talking to me!
            // Chances are they'll send something next.
            // Store this record in our ARP table, then send a reply.
            complete_arp(arp->sha, arp->spa);
            
            if (arp->op == ARP_OP_REQUEST) {
                arp_to(my_addr, arp->spa, arp->sha);
            } else if (arp->op == ARP_OP_REPLY) {
                // already added it!
            } 
            
        }
    }
}

void arp_fill(uint8_t *hwaddr, uint32_t ipaddr) {
    for (uint8_t i = 0; i < NUM_ARP_ENTRIES; i++)
    {
        if (arp_entries[i].ip == ipaddr && arp_entries[i].time > 0) {
            if (arp_entries[i].complete == 1) {
                // found it!
                arp_entries[i].use_time = 255;
                memcpy(hwaddr, arp_entries[i].hwaddr, 6);
            }
            // already looking for it, didn't find it. Broadcast.
            memset(hwaddr, 0xFF, 6);
            return;
        }
    }
    // Didn't find it. Bummer. Let's kick off a request for it.
    uint8_t entry = find_arp_slot();
    arp_entries[entry].complete = 0;
    arp_entries[entry].time = 10;
    arp_entries[entry].ip = ipaddr;

    uint32_t my_addr = ip_get_ip_info()->addr;
    arp_for(my_addr, ipaddr);
    
    // Broadcast this IP packet. We'll eventually get an ARP reply, or snoop on an IP response, whichever comes first.
    memset(hwaddr, 0xFF, 6);
}

// Send arp reply to host.
void arp_to(uint32_t src, uint32_t dst, uint8_t *addr) {
    memset(arp_buff.buff, 0x00, 60);
    arp_buff.len = 60;

    // Ethernet header
    ether_hdr *hdr = (ether_hdr *) arp_buff.buff;
    memcpy(hdr->dest_addr, addr, 6);
    nic_write_mac(hdr->src_addr);
    hdr->ethertype = htons(ETH_P_ARP);

    arp_ether_ipv4 *arp = (arp_ether_ipv4*) (arp_buff.buff + sizeof(ether_hdr));
    arp->htype = htons(ARP_HTYPE_ETHER);
    arp->ptype = htons(ETH_P_IPV4);
    arp->hlen = ETH_ALEN;
    arp->plen = IP_ALEN;
    arp->op = htons(2);
    nic_write_mac(arp->sha);
    memcpy(arp->tha, addr, 6);
    arp->spa = src;
    arp->tpa = dst;

    nic_send(&arp_buff);
}
//Sending ARP reply to fd28660a 4428660a, 48:f1:7f:65:8f:b5netstack tick


// Send arp request
void arp_for(uint32_t src, uint32_t addr) {
    memset(arp_buff.buff, 0x00, 60);
    arp_buff.len = 60;

    // Ethernet header
    ether_hdr *hdr = (ether_hdr *) arp_buff.buff;
    memset(hdr->dest_addr, 0xFF, 6);
    nic_write_mac(hdr->src_addr);
    hdr->ethertype = htons(ETH_P_ARP);

    arp_ether_ipv4 *arp = (arp_ether_ipv4*) (arp_buff.buff + sizeof(ether_hdr));
    arp->htype = htons(ARP_HTYPE_ETHER);
    arp->ptype = htons(ETH_P_IPV4);
    arp->hlen = ETH_ALEN;
    arp->plen = IP_ALEN;
    arp->op = htons(1);
    nic_write_mac(arp->sha);
    memset(arp->tha, 0xFF, 6);
    arp->spa = src;
    arp->tpa = addr;

    nic_send(&arp_buff);
}


// call arp_interval at ~1 second intervals, to do cache work.
void arp_interval() {
    uint32_t my_addr = ip_get_ip_info()->addr;
    // expire existing entries
    for (uint8_t i = 0; i < NUM_ARP_ENTRIES; i++) {
        if(arp_entries[i].time > 0) arp_entries[i].time--;
        if(arp_entries[i].use_time > 0) arp_entries[i].use_time--;

        // look for any incomplete requests with time & attempts left.
        if (arp_entries[i].complete == 0 && arp_entries[i].time > 0) {
            arp_for(my_addr, arp_entries[i].ip);
        }

        // Look for any entries about to expire, still in use (in last ~30 seconds), and preemptively ARP.
        if (arp_entries[i].complete == 1 && arp_entries[i].time < 10 && arp_entries[i].use_time > 220) {
            arp_for(my_addr, arp_entries[i].ip);
        }
    }
}