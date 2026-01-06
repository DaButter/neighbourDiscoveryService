#pragma once

#include <unordered_map>
#include <cstring>
#include <string>
#include <net/if.h>
#include "common.hpp"
#include "utils.hpp"

struct Neighbor {
    NeighborPayload payload;
    char ifName[IFNAMSIZ];
    time_t lastSeen;
};

extern std::unordered_map<std::string, Neighbor> neighbors;

namespace neighbor {
    void store(const uint8_t* buffer, const char* ifname);
    void checkTimeout(const time_t& now);
}
