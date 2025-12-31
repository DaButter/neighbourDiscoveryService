#include <iostream>
#include <iomanip>
#include <cstring>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "neighbor.hpp"

// TEMPORARY - DEBUG SRC MAC ADDR
void print_mac(const unsigned char* mac) {
    std::cout << "Source MAC: ";
    std::cout << std::hex
              << std::setw(2) << std::setfill('0') << (int)mac[0] << ":"
              << std::setw(2) << std::setfill('0') << (int)mac[1] << ":"
              << std::setw(2) << std::setfill('0') << (int)mac[2] << ":"
              << std::setw(2) << std::setfill('0') << (int)mac[3] << ":"
              << std::setw(2) << std::setfill('0') << (int)mac[4] << ":"
              << std::setw(2) << std::setfill('0') << (int)mac[5]
              << std::dec;
    std::cout << "\n";
}

void print_buffer(const unsigned char* buffer, ssize_t n) {
    #define DST_MAC_ADDR_OFFSET 6
    std::cout << "Received packet from "
              << std::hex
              << std::setw(2) << std::setfill('0')
              << (int)buffer[0+DST_MAC_ADDR_OFFSET] << ":"
              << (int)buffer[1+DST_MAC_ADDR_OFFSET] << ":"
              << (int)buffer[2+DST_MAC_ADDR_OFFSET] << ":"
              << (int)buffer[3+DST_MAC_ADDR_OFFSET] << ":"
              << (int)buffer[4+DST_MAC_ADDR_OFFSET] << ":"
              << (int)buffer[5+DST_MAC_ADDR_OFFSET]
              << std::dec
              << "  payload: ";

    // print payload as ASCII
    for (int i = 14; i < n; ++i)
        std::cout << (char)buffer[i];
    std::cout << "\n";
}

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
    print_mac(reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data));

    /* contruct ethernet frame */
    unsigned char dst_mac[MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; // need to test - does this broadcast to all mac addresses?
    unsigned char frame[1500]; // we know that we just need 2 mac addresses + ethertype + payload - will reduce size later

    std::memcpy(frame, dst_mac, MAC_ADDR_LEN);                                // Destination MAC
    std::memcpy(frame + MAC_ADDR_LEN, ifr.ifr_hwaddr.sa_data, MAC_ADDR_LEN);  // Source MAC

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

    /* main loop - listen and send */
    unsigned char buffer[2000]; // we know what to expect - will reduce size later
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
            std::cout << "Sent raw Ethernet HELLO on " << ifname << "\n";
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
        // this reads packets one by one - will need to consider optimizing + think if kernel buffer is not dropping packets for 10 000 neighbours
        ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0); // MSG_DONTWAIT ?
        if (n <= 0) {
            perror("recv");
            continue;
        }

        // if (n < 14) continue;
        print_buffer(buffer, n);
    }

    return 0;
}
