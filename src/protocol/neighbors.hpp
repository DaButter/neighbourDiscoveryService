#pragma once

#include <unordered_map>
#include <cstring>
#include <string>
#include <net/if.h>
#include "../common.hpp"
#include "../utils/utils.hpp"

// think about using std::optional for ipv4 and ipv6 addresses
struct Connection {
    char localIfName[IFNAMSIZ];
    uint8_t remoteMac[MAC_ADDR_LEN];
    uint32_t remoteIpv4;
    uint8_t remoteIpv6[16];
    time_t lastSeen;
};

struct Neighbor {
    std::string machineId; // the key of the neighbor key: machine ID
    std::unordered_map<std::string, Connection> connections; // key: local interface name
};

namespace neighbor {
    extern std::unordered_map<std::string, Neighbor> neighbors; // key: machine ID

    void store(const uint8_t* buffer, const char* ifname);
    void checkTimeout(const time_t& now);
}
