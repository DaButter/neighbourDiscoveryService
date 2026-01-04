#include <cstring>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "utils.hpp"
#include "logger.hpp"

int main(int argc, char* argv[]) {
    /* input handling */
    if (argc != 2) {
        LOG_ERROR("Usage: " << argv[0] << " <interface>");
        return 1;
    }

    const char* ifname = argv[1];
    if (strlen(ifname) >= IFNAMSIZ) {
        LOG_ERROR("Interface name longer than 16 bytes");
        return 1;
    }

    /* layer 2 socket */
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR_DISC));
    if (sockfd < 0) {
        LOG_ERROR("socket() Failed to create raw socket: " << strerror(errno));
        return 1;
    }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    /* get host interface index */
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        LOG_ERROR("ioctl() SIOCGIFINDEX: Failed to get index for interface: " << strerror(errno));
        return 1;
    }
    int ifindex = ifr.ifr_ifindex;

    /* get host MAC address */
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        LOG_ERROR("ioctl() SIOCGIFHWADDR: Failed to get MAC address for interface: " << strerror(errno));
        return 1;
    }

    /* construct ethernet frame */
    const uint8_t* srcMac = reinterpret_cast<const uint8_t*>(ifr.ifr_hwaddr.sa_data);
    const uint8_t dstMac[MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; // need to test - does broadcasting work without promiscous mode?
    debug::printMAC(srcMac);


    size_t frame_len = PAYLOAD_OFFSET + sizeof(NeighborPayload);
    uint8_t frame[frame_len];
    buildEthernetFrame(frame, srcMac, dstMac, ifname);


    /* define where to send/recv data for socket */
    struct sockaddr_ll addr{};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = MAC_ADDR_LEN;
    std::memcpy(addr.sll_addr, dstMac, MAC_ADDR_LEN);

    // binded to a specific interface
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind() Failed to bind socket to interface: " << strerror(errno));
        return 1;
    }

    /* main loop - listen and send */
    uint8_t inBuf[frame_len];
    time_t last_send_time = time(nullptr);
    const int SEND_INTERVAL_SEC = 5;

    struct timeval timeout{};

    LOG_INFO("Neighbour Discovery Service started on interface: " << ifname); // one interface for now
    while (true) {
        // calculate time until next send
        time_t now = time(nullptr);
        time_t time_to_next_send = SEND_INTERVAL_SEC - (now - last_send_time);

        /* send data */
        if (time_to_next_send <= 0) {
            if (sendto(sockfd, frame, frame_len, 0,
                       (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                LOG_ERROR("sendto() failed: " << strerror(errno));
                return 1;
            }
            LOG_DEBUG("Sent packet to neighbour on: " << ifname);
            last_send_time = time(nullptr);
            time_to_next_send = SEND_INTERVAL_SEC;
        }

        /* wait for incoming packets */
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeout.tv_sec = time_to_next_send;

        int ret = select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);
        if (ret < 0) {
            perror("select");
            return 1;
        } else if (ret == 0) {
            // timeout, loop to send
            continue;
        }

        /* read incoming packet */
        // this reads packets one by one - will need to consider optimizing + think if kernel inBuf is not dropping packets for 10 000 neighbours
        ssize_t n = recv(sockfd, inBuf, sizeof(inBuf), 0); // MSG_DONTWAIT ?
        if (n <= 0) {
            perror("recv");
            continue;
        }

        // while processing packets, other packets may arrive - they will be inBufed by kernel in a queue
        // receive inBuf is small: 212992 bytes on my machine
        debug::printFrameData(inBuf);
        storeNeighbor(inBuf, n, ifname);
        timeoutNeighbors();
    }

    return 0;
}
