#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <unordered_map>
#include <net/if.h>
#include <arpa/inet.h>

#define MAC_ADDR_LEN 6
#define ETH_TYPE_OFFSET 12
#define PAYLOAD_OFFSET 14

static constexpr uint16_t ETH_P_NEIGHBOR_DISC = 0x88B5;

void printMAC(const uint8_t* mac);

void printBuffer(const uint8_t* buffer,
                 ssize_t n);

void buildEthernetFrame(uint8_t* frame,
                        const uint8_t* src_mac,
                        const uint8_t* dst_mac);

// this is what we send/get to/from neighbors
struct NeighborPayload {
    // uint8_t mac[6]; // possibly redundant
    uint32_t ipv4;
    uint64_t timestamp;
} __attribute__((packed));

// this is what we store per neighbor
struct Neighbour {
    NeighborPayload payload; // neighbor info
    char ifName[IFNAMSIZ];   // interface name where this neighbor was seen
} __attribute__((packed));

static_assert(sizeof(NeighborPayload) == 12, "NeighborPayload must be 12 bytes");

// std::unordered_map<std::string, Neighbour> neighbors; // MAC, data

// std::string key = std::string(reinterpret_cast<char*>(mac), 6);
