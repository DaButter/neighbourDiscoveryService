#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define SRC_MAC_OFFSET 6
#define PAYLOAD_OFFSET 14
#define MAC_ADDR_LEN 6

static constexpr uint16_t ETH_P_NEIGHBOR_DISC = 0x88B5;

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

    if (sendto(sockfd, frame, frame_len, 0,
               (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        return 1;
    }

    std::cout << "Sent raw Ethernet HELLO on " << ifname << "\n";
    return 0;
}
