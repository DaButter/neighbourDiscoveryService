#include "server.hpp"

namespace ipc {
    int server_fd = -1;

    bool initServer() {
        unlink(SOCKET_PATH);

        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd < 0) {
            LOG_ERROR("Failed to create IPC socket: " << strerror(errno));
            return false;
        }

        // set non-blocking flag
        int flags = fcntl(server_fd, F_GETFL, 0);
        if (flags < 0 || fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            LOG_ERROR("Failed to set non-blocking: " << strerror(errno));
            close(server_fd);
            server_fd = -1;
            return false;
        }

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR("Failed to bind IPC socket: " << strerror(errno));
            close(server_fd);
            server_fd = -1;
            return false;
        }

        if (listen(server_fd, 5) < 0) {
            LOG_ERROR("Failed to listen on IPC socket: " << strerror(errno));
            close(server_fd);
            server_fd = -1;
            return false;
        }

        LOG_INFO("IPC server listening on " << SOCKET_PATH);
        return true;
    }


    /* Serialize neighbor data into binary protocol format:
       Protocol structure:
       [neighbor_count:4 bytes]
       For each neighbor:
         [NeighborInfo:36 bytes]
         [ConnectionInfo:42 bytes] × connectionCount

       Example: 2 neighbors, first has 2 connections, second has 1:
       [0x02,0x00,0x00,0x00] [NeighborInfo] [ConnInfo] [ConnInfo] [NeighborInfo] [ConnInfo]
    */
    static void handleClient(int client_fd) {
        size_t total_conns = 0;
        for (const auto& [_, neighbor] : neighbor::neighbors) {
            total_conns += neighbor.connections.size();
        }

        size_t buffer_size = sizeof(uint32_t) +
                             neighbor::neighbors.size() * sizeof(NeighborInfo) +
                             total_conns * sizeof(ConnectionInfo);

        static std::vector<uint8_t> buffer;
        buffer.clear();
        buffer.reserve(buffer_size);

        // form buffer
        uint32_t neighborCount = neighbor::neighbors.size();
        uint8_t* count_ptr = reinterpret_cast<uint8_t*>(&neighborCount);
        buffer.insert(buffer.end(), count_ptr, count_ptr + sizeof(neighborCount));

        for (const auto& [machineId, neighbor] : neighbor::neighbors) {
            NeighborInfo info{};
            std::memcpy(info.machineId, machineId.c_str(), MACHINE_ID_LEN);
            info.connectionCount = neighbor.connections.size();

            uint8_t* info_ptr = reinterpret_cast<uint8_t*>(&info);
            buffer.insert(buffer.end(), info_ptr, info_ptr + sizeof(info));

            for (const auto& [ifname, conn] : neighbor.connections) {
                ConnectionInfo connInfo{};
                std::strncpy(connInfo.localIfName, conn.localIfName, IFNAMSIZ);
                std::memcpy(connInfo.remoteMac, conn.remoteMac, MAC_ADDR_LEN);
                connInfo.remoteIpv4 = conn.remoteIpv4;
                std::memcpy(connInfo.remoteIpv6, conn.remoteIpv6, 16);

                const uint8_t* conn_ptr = reinterpret_cast<const uint8_t*>(&connInfo);
                buffer.insert(buffer.end(), conn_ptr, conn_ptr + sizeof(connInfo));
            }
        }

        // send buffer
        size_t total_sent = 0;
        while (total_sent < buffer.size()) {
            ssize_t sent = send(client_fd, buffer.data() + total_sent, buffer.size() - total_sent, 0);
            if (sent <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // socket buffer full, send more
                    continue;
                }
                LOG_ERROR("Failed to send to IPC client: " << strerror(errno));
                break;
            }
            total_sent += sent;
        }

        close(client_fd);
    }

    void checkClients() {
        if (server_fd < 0) return;

        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            LOG_INFO("IPC client connected");
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
