#pragma once

#include "utils.hpp"
#include <vector>

struct ActiveEthInterface {
    std::string name;
    int sockfd;
    int ifindex;
    uint8_t mac[MAC_ADDR_LEN];
    uint32_t ipv4;
    uint8_t ipv6[16];
    time_t last_send_time; // do we need this?
    uint8_t send_frame[PAYLOAD_OFFSET + sizeof(NeighborPayload)];
};

struct EthInterface {
    std::string name;
    int ifindex;
    uint8_t mac[MAC_ADDR_LEN];
    uint32_t ipv4;
    uint8_t ipv6[16];
    bool has_ipv4 = false; // not sure if i will need this
    bool has_ipv6 = false;
};

// map of active ethernet interfaces being monitored
extern std::unordered_map<std::string, ActiveEthInterface> activeEthInterfaces;

// interface discovery
std::unordered_map<std::string, EthInterface> discoverEthInterfaces();
bool isInterfaceUp(const char* ifname);
void addInterface(const std::string& ifname);
void removeInterface(const std::string& ifname);
void checkEthInterfaces();