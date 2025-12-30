
#pragma once

#include <cstdint>
#include <unordered_map>

#define SRC_MAC_OFFSET 6
#define PAYLOAD_OFFSET 14
#define MAC_ADDR_LEN 6

static constexpr uint16_t ETH_P_NEIGHBOR_DISC = 0x88B5;

struct ConnectionKey {
    int ifindex;                         // local interface
    unsigned char mac[MAC_ADDR_LEN];     // neighbor MAC
};

// bool operator==(const ConnectionKey& a, const ConnectionKey& b) {
//     return a.ifindex == b.ifindex &&
//            std::memcmp(a.mac, b.mac, MAC_ADDR_LEN) == 0;
// }

struct ConnectionKeyHash {
    std::size_t operator()(const ConnectionKey& k) const {
        std::size_t h = std::hash<int>()(k.ifindex);
        for (int i = 0; i < MAC_ADDR_LEN; ++i) {
            h ^= std::hash<unsigned char>()(k.mac[i] + 0x9e3779b9 + (h<<6) + (h>>2));
        }
        return h;
    }
};

struct ConnectionData {
    unsigned char ip4[4];     // or 16 for ipv6, or both
    uint64_t last_seen_ms;    // to enforce 30s timeout
};

std::unordered_map<ConnectionKey, ConnectionData, ConnectionKeyHash> connections;
