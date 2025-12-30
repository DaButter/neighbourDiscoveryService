#include <iostream>
#include <iomanip>
#include <cstring>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "neighbor.hpp"
#include <thread> // tmp for sleep

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

    // TEMPORARY - DEBUG SRC MAC ADDR
    std::cout << "Source MAC: "
              << std::hex
              << static_cast<int>(static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[0])) << ":"
              << static_cast<int>(static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[1])) << ":"
              << static_cast<int>(static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[2])) << ":"
              << static_cast<int>(static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[3])) << ":"
              << static_cast<int>(static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[4])) << ":"
              << static_cast<int>(static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[5])) << std::dec << "\n";

    /* contruct ethernet frame */
    unsigned char dst_mac[MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; // need to test - does this broadcast to all mac addresses?
    unsigned char frame[1500];

    std::memcpy(frame, dst_mac, MAC_ADDR_LEN);                                 // Destination MAC
    std::memcpy(frame + SRC_MAC_OFFSET, ifr.ifr_hwaddr.sa_data, MAC_ADDR_LEN); // Source MAC

    frame[12] = (ETH_P_NEIGHBOR_DISC >> 8) & 0xff;
    frame[13] = (ETH_P_NEIGHBOR_DISC) & 0xff;

    const char* payload = "HELLO_FROM_VM1_AUSTRIS";

    std::memcpy(frame + PAYLOAD_OFFSET, payload, std::strlen(payload));
    size_t frame_len = PAYLOAD_OFFSET + std::strlen(payload);

    /* send ethernet frame */
    struct sockaddr_ll addr{};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = MAC_ADDR_LEN;
    std::memcpy(addr.sll_addr, dst_mac, MAC_ADDR_LEN);

    // bind for listening replies (not strictly necessary for sending)
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    unsigned char buffer[2000];
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // send every 5 seconds

        /* send data */
        if (sendto(sockfd, frame, frame_len, 0,
                   (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("sendto");
            return 1;
        }

        std::cout << "Sent raw Ethernet HELLO on " << ifname << "\n";

        /* receive reply */
        ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            perror("recv");
            continue;
        }

        if (n < 14) continue;
        unsigned char* src = buffer + 6;

        std::cout << "Received packet from "
                  << std::hex
                  << std::setw(2) << std::setfill('0')
                  << (int)src[0] << ":"
                  << (int)src[1] << ":"
                  << (int)src[2] << ":"
                  << (int)src[3] << ":"
                  << (int)src[4] << ":"
                  << (int)src[5]
                  << std::dec
                  << "  payload: ";

        // print payload as ASCII
        for (int i = 14; i < n; ++i)
            std::cout << (char)buffer[i];
        std::cout << "\n";
    }

    return 0;
}
