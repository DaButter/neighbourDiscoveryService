#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

static constexpr uint16_t MY_ETHERTYPE = 0x88B5;

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

    int sock = socket(AF_PACKET, SOCK_RAW, htons(MY_ETHERTYPE));
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // Get interface index
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return 1;
    }
    int ifindex = ifr.ifr_ifindex;

    // Get source MAC address
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        return 1;
    }
    unsigned char src_mac[6];
    std::memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);

    // Destination: broadcast
    unsigned char dst_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

    // Construct Ethernet frame
    unsigned char frame[1500];
    std::memcpy(frame, dst_mac, 6);
    std::memcpy(frame + 6, src_mac, 6);

    // EtherType
    frame[12] = (MY_ETHERTYPE >> 8) & 0xff;
    frame[13] = (MY_ETHERTYPE) & 0xff;

    // Payload: "HELLO"
    const char* payload = "HELLO_FROM_VM1";
    size_t payload_len = std::strlen(payload);

    std::memcpy(frame + 14, payload, payload_len);

    size_t frame_len = 14 + payload_len;

    // Destination socket address
    struct sockaddr_ll addr{};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = 6;
    std::memcpy(addr.sll_addr, dst_mac, 6);

    if (sendto(sock, frame, frame_len, 0,
               (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        return 1;
    }

    std::cout << "Sent raw Ethernet HELLO on " << ifname << "\n";
    return 0;
}
