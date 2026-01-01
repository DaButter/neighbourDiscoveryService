#include <cstring>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "utils.hpp"

int main(int argc, char* argv[]) {
    /* input handling */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <interface>\n";
        return 1;
    }

    const char* ifname = argv[1];
    if (strlen(ifname) >= IFNAMSIZ) {
        std::cerr << "ERROR: Interface name longer than 16 bytes\n";
        return 1;
    }

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR_DISC));
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    /* get host interface index */
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return 1;
    }
    int ifindex = ifr.ifr_ifindex;

    /* get host MAC address */
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        return 1;
    }

    /* construct ethernet frame */
    const uint8_t* src_mac = reinterpret_cast<const uint8_t*>(ifr.ifr_hwaddr.sa_data);
    const uint8_t dst_mac[MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; // need to test - does this broadcast to all mac addresses?
    printMAC(src_mac);


    size_t frame_len = PAYLOAD_OFFSET + sizeof(NeighborPayload);
    uint8_t frame[frame_len];
    buildEthernetFrame(frame, src_mac, dst_mac);


    /* define where to send/recv data for socket */
    struct sockaddr_ll addr{};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = MAC_ADDR_LEN;
    std::memcpy(addr.sll_addr, dst_mac, MAC_ADDR_LEN);

    // binded to a specific interface
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    /* main loop - listen and send */
    uint8_t inBuf[frame_len];
    time_t last_send_time = time(nullptr);
    const int SEND_INTERVAL_SEC = 5;

    struct timeval timeout{};

    while (true) {
        // calculate time until next send
        time_t now = time(nullptr);
        time_t time_to_next_send = SEND_INTERVAL_SEC - (now - last_send_time);

        /* send data */
        if (time_to_next_send <= 0) {
            if (sendto(sockfd, frame, frame_len, 0,
                       (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                perror("sendto");
                return 1;
            }
            std::cout << "Sent packets to neighbour on: " << ifname << "\n";
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
        // if (n < 14) continue;
        printBuffer(inBuf, n);
    }

    return 0;
}
