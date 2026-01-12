#include "ipc.hpp"
#include "../common.hpp"
#include <iostream>
#include <iomanip>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

void printIPv4(uint32_t ipv4) {
    if (ipv4 == 0) {
        std::cout << "none";
    } else {
        struct in_addr addr;
        addr.s_addr = ipv4;
        std::cout << inet_ntoa(addr);
    }
}

void printIPv6(const uint8_t* ipv6) {
    bool has_ipv6 = false;
    for (int i = 0; i < 16; ++i) {
        if (ipv6[i] != 0) {
            has_ipv6 = true;
            break;
        }
    }

    if (!has_ipv6) {
        std::cout << "none";
    } else {
        for (int i = 0; i < 16; i += 2) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ipv6[i]
                      << std::setw(2) << (int)ipv6[i+1];
            if (i < 14) std::cout << ":";
        }
        std::cout << std::dec;
    }
}

void printMAC(const uint8_t* mac) {
    std::cout << std::hex << std::setfill('0')
              << std::setw(2) << (int)mac[0] << ":"
              << std::setw(2) << (int)mac[1] << ":"
              << std::setw(2) << (int)mac[2] << ":"
              << std::setw(2) << (int)mac[3] << ":"
              << std::setw(2) << (int)mac[4] << ":"
              << std::setw(2) << (int)mac[5]
              << std::dec;
}

int main() {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to service. Is it running?" << std::endl;
        close(sockfd);
        return 1;
    }

    // receive neighbor count
    uint32_t neighbor_count = 0;
    ssize_t n = recv(sockfd, &neighbor_count, sizeof(neighbor_count), 0);
    if (n != sizeof(neighbor_count)) {
        std::cerr << "Failed to receive neighbor count" << std::endl;
        close(sockfd);
        return 1;
    }

    // receive each neighbor
    for (uint32_t i = 0; i < neighbor_count; ++i) {
        NeighborInfo info;
        recv(sockfd, &info, sizeof(info), 0);

        std::cout << "\n=== Neighbor " << (i + 1) << " ===\n";
        std::cout << "Machine ID: ";
        std::cout.write(info.machineId, MACHINE_ID_LEN);

        // receive each connection
        for (uint32_t j = 0; j < info.connectionCount; ++j) {
            ConnectionInfo conn;
            recv(sockfd, &conn, sizeof(conn), 0);

            std::cout << "\n  Connection " << (j + 1) << ":\n";
            std::cout << "\tLocal interface: " << conn.localIfName << '\n';
            std::cout << "\tRemote MAC:      ";
            printMAC(conn.remoteMac);
            std::cout << '\n';
            std::cout << "\tRemote IPv4:     ";
            printIPv4(conn.remoteIpv4);
            std::cout << '\n';
            std::cout << "\tRemote IPv6:     ";
            printIPv6(conn.remoteIpv6);
            std::cout << '\n';
        }
    }

    if (neighbor_count == 0) {
        std::cout << "No active neighbors found." << std::endl;
    } else {
        std::cout << "\nTotal neighbors: " << neighbor_count << std::endl;
    }

    close(sockfd);
    return 0;
}
