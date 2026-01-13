#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include "../utils/utils.hpp"
#include "../protocol/neighbors.hpp"
#include "../common.hpp"

inline constexpr const char* SOCKET_PATH = "/tmp/neighbor_discovery.sock";
inline constexpr int MAX_CLIENTS = 5; // is this needed?

struct NeighborInfo {
    char machineId[MACHINE_ID_LEN];
    uint32_t connectionCount;
} __attribute__((packed));

struct ConnectionInfo {
    char localIfName[IFNAMSIZ];
    uint8_t remoteMac[MAC_ADDR_LEN];
    uint32_t remoteIpv4;
    uint8_t remoteIpv6[16];
} __attribute__((packed));

namespace ipc {
    extern int server_fd;
    bool initServer();
    void handleClient(int client_fd);
    void checkClients();
    void cleanup();
}
