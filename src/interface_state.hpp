#pragma once

#include "utils.hpp"
#include <vector>

struct EthInterface {
    std::string name;
    int ifindex = 0;
    uint8_t mac[MAC_ADDR_LEN] = {};
    uint32_t ipv4 = 0;
    uint8_t ipv6[16] = {};

    bool hasIPv4() const {
        return ipv4 != 0;
    }

    bool hasIPv6() const {
        for (int i = 0; i < 16; ++i) {
            if (ipv6[i] != 0) return true;
        }
        return false;
    }
};

struct ActiveEthInterface {
    EthInterface ifData;
    int sockfd;
    uint8_t send_frame[PAYLOAD_OFFSET + sizeof(NeighborPayload)];
    struct sockaddr_ll send_addr = {};
};

// map of active ethernet interfaces being monitored
extern std::unordered_map<std::string, ActiveEthInterface> activeEthInterfaces;

// interface discovery
std::unordered_map<std::string, EthInterface> discoverEthInterfaces();
void addInterface(const std::string& ifname);
void checkEthInterfaces();