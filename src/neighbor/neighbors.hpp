#pragma once

#include <unordered_map>
#include <cstring>
#include <string>
#include <net/if.h>
#include "../common.hpp"
#include "../utils/utils.hpp"

struct Connection {
    char localIfName[IFNAMSIZ];
    uint8_t remoteMac[MAC_ADDR_LEN];
    uint32_t remoteIpv4;
    uint8_t remoteIpv6[16];
    time_t lastSeen;
};

struct Neighbor {
    std::string machineId;
    std::unordered_map<std::string, Connection> connections;

    Neighbor() {
        connections.reserve(4);
    }
};

namespace neighbor {
    extern std::unordered_map<std::string, Neighbor> neighbors;

    void init();
    void store(const uint8_t* buffer, const char* ifname);
    void checkTimeout(const time_t& now);
}
