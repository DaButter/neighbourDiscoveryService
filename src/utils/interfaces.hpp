#pragma once

#include <unordered_map>
#include <string>
#include <net/if.h>
#include <cstring>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <unistd.h>
#include "../common.hpp"
#include "../utils/utils.hpp"

struct EthInterface {
    std::string name;
    int ifindex = 0;
    uint8_t mac[MAC_ADDR_LEN] = {};
    uint32_t ipv4 = 0;
    uint8_t ipv6[16] = {};

    bool hasIPv4() const { return ipv4 != 0; }

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


namespace interfaces {
    // map of active ethernet interfaces being monitored
    extern std::unordered_map<std::string, ActiveEthInterface> activeEthInterfaces;

    std::unordered_map<std::string, EthInterface> discover();
    void add(const EthInterface& ethInterface, const uint8_t* machineId);
    void update(const EthInterface& ethInterface, const uint8_t* machineId);
    void checkAndUpdate(const uint8_t* machineId);
}