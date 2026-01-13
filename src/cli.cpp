#include <iostream>
#include <iomanip>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "common.hpp"
#include "ipc/server.hpp"
#include "utils/utils.hpp"

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
            utils::printMAC(conn.remoteMac);
            std::cout << '\n';
            std::cout << "\tRemote IPv4:     ";
            utils::printIPv4(conn.remoteIpv4);
            std::cout << '\n';
            std::cout << "\tRemote IPv6:     ";
            utils::printIPv6(conn.remoteIpv6);
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
