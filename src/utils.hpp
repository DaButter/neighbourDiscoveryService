#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <unordered_map>
#include <net/if.h>
#include <arpa/inet.h>
#include <unordered_map>

#define MAC_ADDR_LEN 6
#define ETH_TYPE_OFFSET 12
#define PAYLOAD_OFFSET 14

static constexpr uint16_t ETH_P_NEIGHBOR_DISC = 0x88B5;

void printMAC(const uint8_t* mac);
void printFrameData(const uint8_t* buffer, ssize_t n);
void buildEthernetFrame(uint8_t* frame, const uint8_t* srcMac, const uint8_t* dstMac);
void storeNeighbor(const uint8_t* buffer, ssize_t n, const char* ifname);

// payload structure sent to/from neighbors
struct NeighborPayload {
    uint32_t ipv4;
    uint8_t ipv6[16];
} __attribute__((packed));

// this is what we store per neighbor
struct Neighbour {
    NeighborPayload payload; // neighbor info
    char ifName[IFNAMSIZ];   // interface name where this neighbor was seen
    time_t lastSeen;
};

static_assert(sizeof(NeighborPayload) == 20, "NeighborPayload must be 20 bytes");

// map of neighbors: key = MAC address string, value = Neighbour struct
extern std::unordered_map<std::string, Neighbour> neighbors;
