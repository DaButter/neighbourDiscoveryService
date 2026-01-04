#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <unordered_map>
#include <net/if.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/types.h>

// maybe redefine these as uint or smth later for cleanup
#define MAC_ADDR_LEN 6
#define ETH_TYPE_OFFSET 12
#define PAYLOAD_OFFSET 14

#define NEIGHBOR_EXPIRY_SECONDS 30

/*
    IEEE Std 802 - Local Experimental Ethertype, so we can use this one safely
    https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml
*/
static constexpr uint16_t ETH_P_NEIGHBOR_DISC = 0x88B5;

static const uint8_t broadcastMac[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

namespace debug {
    void printMAC(const uint8_t* mac);
    void printMACFromString(const std::string& macStr);
    void printFrameData(const uint8_t* buffer);
}

void buildEthernetFrame(uint8_t* frame, const uint8_t* srcMac, uint32_t ipv4, const uint8_t* ipv6);

void storeNeighbor(const uint8_t* buffer, ssize_t n, const char* ifname);
void timeoutNeighbors();

std::string macToString(const uint8_t* mac);

// interface may have multiple addresses - for now we send just the first one we get
// will need to improve later...
struct NeighborPayload {
    uint32_t ipv4;
    uint8_t ipv6[16];
} __attribute__((packed));

struct Neighbour {
    NeighborPayload payload; 
    char ifName[IFNAMSIZ];   // interface name where this neighbor was seen
    time_t lastSeen;
};

static_assert(sizeof(NeighborPayload) == 20, "NeighborPayload must be 20 bytes");

extern std::unordered_map<std::string, Neighbour> neighbors;
