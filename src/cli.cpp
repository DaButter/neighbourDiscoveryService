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

    uint32_t neighborCount = 0;
    if (recv(sockfd, &neighborCount, sizeof(neighborCount), MSG_WAITALL) != sizeof(neighborCount)) {
        std::cerr << "Failed to receive neighbor count" << std::endl;
        close(sockfd);
        return 1;
    }

    for (uint32_t i = 0; i < neighborCount; ++i) {
        NeighborInfo info;
        if (recv(sockfd, &info, sizeof(info), MSG_WAITALL) != sizeof(info)) {
            std::cerr << "Failed to receive neighbor info" << std::endl;
            close(sockfd);
            return 1;
        }

        std::cout << "\n=== Neighbor " << (i + 1) << " ===\n";
        std::cout << "Machine ID: ";
        std::cout.write(info.machineId, MACHINE_ID_LEN);
        std::cout << '\n';

        for (uint32_t j = 0; j < info.connectionCount; ++j) {
            ConnectionInfo conn;
            if (recv(sockfd, &conn, sizeof(conn), MSG_WAITALL) != sizeof(conn)) {
                std::cerr << "Failed to receive connection info" << std::endl;
                close(sockfd);
                return 1;
            }

            std::cout << "\n  Connection " << (j + 1) << ":\n";
            std::cout << "    Local interface: " << conn.localIfName << '\n';
            std::cout << "    Remote MAC:      ";
            utils::printMAC(conn.remoteMac);
            std::cout << '\n';
            std::cout << "    Remote IPv4:     ";
            utils::printIPv4(conn.remoteIpv4);
            std::cout << '\n';
            std::cout << "    Remote IPv6:     ";
            utils::printIPv6(conn.remoteIpv6);
            std::cout << '\n';
        }
    }


    if (neighborCount == 0) {
        std::cout << "No active neighbors found." << std::endl;
    } else {
        std::cout << "\nTotal neighbors: " << neighborCount << std::endl;
    }

    close(sockfd);
    return 0;
}
