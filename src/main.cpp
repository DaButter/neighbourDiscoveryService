#include <cstring>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <vector>
#include "utils.hpp"
#include "logger.hpp"
#include "interface_state.hpp"

int main() {
    LOG_INFO("Neighbour Discovery Service starting...");

    /* initial interface discovery */
    checkEthInterfaces();
    if (activeEthInterfaces.empty()) {
        LOG_ERROR("No active Ethernet interfaces found!");
        return 1;
    }

    const int SEND_INTERVAL_SEC = 5;
    const int INTERFACE_CHECK_INTERVAL_SEC = 10;
    time_t last_interface_check = time(nullptr);

    uint8_t recvBuf[PAYLOAD_OFFSET + sizeof(NeighborPayload)]; // for now like this, but will need to handle multiple IPs later

    LOG_INFO("Service running on " << activeEthInterfaces.size() << " interface(s)");

    /* main loop */
    while (true) {
        time_t now = time(nullptr);

        /* periodically check for interface changes */
        if (now - last_interface_check >= INTERFACE_CHECK_INTERVAL_SEC) {
            checkEthInterfaces();
            last_interface_check = now;
        }

        /* send on all interfaces that need it */
        for (auto& [ifname, ethInterface] : activeEthInterfaces) {
            if (now - ethInterface.last_send_time >= SEND_INTERVAL_SEC) {

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
                    ethInterface.last_send_time = now;
                }
            }
        }

        /* calculate timeout for select() */
        time_t min_time_to_send = SEND_INTERVAL_SEC;
        for (const auto& [ifname, ethInterface] : activeEthInterfaces) {
            time_t time_to_send = SEND_INTERVAL_SEC - (now - ethInterface.last_send_time);
            if (time_to_send < min_time_to_send) {
                min_time_to_send = time_to_send;
            }
        }
        if (min_time_to_send < 0) min_time_to_send = 0;

        /* build fd_set for all interfaces */
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = 0;

        for (const auto& [ifname, ethInterface] : activeEthInterfaces) {
            FD_SET(ethInterface.sockfd, &readfds);
            if (ethInterface.sockfd > max_fd) {
                max_fd = ethInterface.sockfd;
            }
        }

        /* wait for incoming packets */
        struct timeval timeout{};
        timeout.tv_sec = min_time_to_send;
        // timeout.tv_usec = 0;

        int ret = select(max_fd + 1, &readfds, nullptr, nullptr, &timeout);
        if (ret < 0) {
            LOG_ERROR("select() failed: " << strerror(errno));
            continue;
        } else if (ret == 0) {
            /* data waiting timeout - loop to send */
            continue;
        }

        /* check which sockets have data */
        /* this reads packets one by one for each interface - will need to consider optimizing + think if kernel inBuf is not dropping packets for 10 000 neighbours */
        /* quite bad tbh */
        for (auto& [ifname, ethInterface] : activeEthInterfaces) {
            if (FD_ISSET(ethInterface.sockfd, &readfds)) {
                ssize_t n = recv(ethInterface.sockfd, recvBuf, sizeof(recvBuf), 0); // MSG_DONTWAIT?
                if (n <= 0) {
                    LOG_ERROR("recv() failed on " << ifname << ": " << strerror(errno));
                    continue;
                }

                /* while processing packets, other packets may arrive - they will be inBufed by kernel in a queue */
                /* receive inBuf is small: 212992 bytes on my machine */
                debug::printFrameData(recvBuf);
                storeNeighbor(recvBuf, n, ifname.c_str());
            }
        }

        timeoutNeighbors();
    }


    // cleanup
    for (auto& [ifname, ethInterface] : activeEthInterfaces) {
        close(ethInterface.sockfd);
    }

    return 0;
}