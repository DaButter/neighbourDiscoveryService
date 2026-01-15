#include "utils/utils.hpp"
#include "utils/interfaces.hpp"
#include "protocol/neighbors.hpp"
#include "ipc/server.hpp"

int main() {
    LOG_INFO("Neighbor Discovery Service starting...");

    /* get machine UID */
    uint8_t machineId[MACHINE_ID_LEN];
    if (!utils::getMachineId(machineId)) {
        LOG_ERROR("Failed to get machine ID, exiting");
        return 1;
    }

    /* initialize IPC server */
    if (!ipc::initServer()) {
        LOG_ERROR("Failed to initialize IPC server");
        ipc::cleanup();
        return 1;
    }

    /* initial interface discovery */
    interfaces::checkAndUpdate(machineId);
    if (interfaces::monitoredEthInterfaces.empty()) {
        LOG_ERROR("No active Ethernet interfaces found!");
        ipc::cleanup();
        return 1;
    }

    const int SEND_INTERVAL_SEC = 5;
    time_t last_send_time = 0;
    uint8_t recvBuf[PAYLOAD_OFFSET + sizeof(NeighborPayload)];

    LOG_INFO("Service running on " << interfaces::monitoredEthInterfaces.size() << " interface(s)");

    /* main loop */
    while (true) {
        time_t now = time(nullptr);
        neighbor::checkTimeout(now);

        /* check IPC clients */
        ipc::checkClients();

        /* check interfaces and send */
        if (now - last_send_time >= SEND_INTERVAL_SEC) {
            interfaces::checkAndUpdate(machineId);

            if (interfaces::monitoredEthInterfaces.empty()) {
                LOG_WARN("No active Ethernet interfaces found, skipping send");
                last_send_time = now;
                continue;
            }

            for (auto& [ifname, ethInterface] : interfaces::monitoredEthInterfaces) {
                ssize_t sent = sendto(ethInterface.sockfd,
                                      ethInterface.send_frame,
                                      sizeof(ethInterface.send_frame),
                                      0,
                                      (struct sockaddr*)&ethInterface.send_addr,
                                      sizeof(ethInterface.send_addr));
                if (sent < 0) {
                    LOG_ERROR("sendto() failed on " << ifname << ": " << strerror(errno));
                } else {
                    LOG_DEBUG("Sent packet on " << ifname);
                }
            }

            last_send_time = now;
        }

        /* calculate timeout for select() */
        time_t time_until_send = SEND_INTERVAL_SEC - (now - last_send_time);
        if (time_until_send < 0) time_until_send = 0;

        /* build fd_set for all interfaces */
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = 0;

        for (const auto& [ifname, ethInterface] : interfaces::monitoredEthInterfaces) {
            FD_SET(ethInterface.sockfd, &readfds);
            if (ethInterface.sockfd > max_fd) {
                max_fd = ethInterface.sockfd;
            }
        }

        // add IPC server socket
        if (ipc::server_fd >= 0) {
            FD_SET(ipc::server_fd, &readfds);
            if (ipc::server_fd > max_fd) {
                max_fd = ipc::server_fd;
            }
        }

        /* wait for incoming packets */
        struct timeval timeout{};
        timeout.tv_sec = time_until_send;
        // timeout.tv_usec = 0;

        int ret = select(max_fd + 1, &readfds, nullptr, nullptr, &timeout);
        if (ret < 0) {
            LOG_ERROR("select() failed: " << strerror(errno));
            continue;
        } else if (ret == 0) {
            /* data waiting timeout - loop to send */
            continue;
        }

        if (ipc::server_fd >= 0 && FD_ISSET(ipc::server_fd, &readfds)) {
            ipc::checkClients();
        }

        /* check which sockets have data */
        /* this reads packets one by one for each interface - will need to consider optimizing + think if kernel inBuf is not dropping packets for 10 000 neighbors */
        /* quite bad tbh */
        for (auto& [ifname, ethInterface] : interfaces::monitoredEthInterfaces) {
            if (FD_ISSET(ethInterface.sockfd, &readfds)) {
                ssize_t n = recv(ethInterface.sockfd, recvBuf, sizeof(recvBuf), 0); // MSG_DONTWAIT?
                if (n <= 0) {
                    LOG_ERROR("recv() failed on " << ifname << ": " << strerror(errno));
                    continue;
                }

                /* while processing packets, other packets may arrive - they will be inBufed by kernel in a queue */
                /* receive inBuf is small: 212992 bytes on my machine */
                neighbor::store(recvBuf, ifname.c_str());
            }
        }
    }

    for (auto& [ifname, ethInterface] : interfaces::monitoredEthInterfaces) {
        close(ethInterface.sockfd);
    }
    ipc::cleanup();

    return 0;
}