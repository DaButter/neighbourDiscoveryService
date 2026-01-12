#include "ipc.hpp"
#include "../neighbors.hpp"
#include "../utils.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace ipc {
    int server_fd = -1;

    bool initServer() {
        unlink(SOCKET_PATH);

        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd < 0) {
            LOG_ERROR("Failed to create IPC socket: " << strerror(errno));
            return false;
        }

        // set non-blocking
        int flags = fcntl(server_fd, F_GETFL, 0);
        fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR("Failed to bind IPC socket: " << strerror(errno));
            close(server_fd);
            server_fd = -1;
            return false;
        }

        if (listen(server_fd, MAX_CLIENTS) < 0) {
            LOG_ERROR("Failed to listen on IPC socket: " << strerror(errno));
            close(server_fd);
            server_fd = -1;
            return false;
        }

        LOG_INFO("IPC server listening on " << SOCKET_PATH);
        return true;
    }

    void handleClient(int client_fd) {
        // send neighbor count
        uint32_t neighborCount = neighbor::neighbors.size();
        send(client_fd, &neighborCount, sizeof(neighborCount), 0);

        // send each neighbor
        for (const auto& [machineId, neighbor] : neighbor::neighbors) {
            // send neighbor info
            NeighborInfo info;
            std::memcpy(info.machineId, machineId.c_str(), MACHINE_ID_LEN);
            info.connectionCount = neighbor.connections.size();
            send(client_fd, &info, sizeof(info), 0);

            // send each connection
            for (const auto& [ifname, conn] : neighbor.connections) {
                ConnectionInfo connInfo;
                std::strncpy(connInfo.localIfName, conn.localIfName, IFNAMSIZ);
                std::memcpy(connInfo.remoteMac, conn.remoteMac, MAC_ADDR_LEN);
                connInfo.remoteIpv4 = conn.remoteIpv4;
                std::memcpy(connInfo.remoteIpv6, conn.remoteIpv6, 16);

                send(client_fd, &connInfo, sizeof(connInfo), 0);
            }
        }

        close(client_fd);
    }

    void checkClients() {
        if (server_fd < 0) return;

        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            LOG_DEBUG("IPC client connected");
            handleClient(client_fd);
        }
    }

    void cleanup() {
        if (server_fd >= 0) {
            close(server_fd);
            unlink(SOCKET_PATH);
        }
    }
}
