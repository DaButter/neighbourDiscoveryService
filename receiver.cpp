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
#include <iomanip>

static constexpr uint16_t MY_ETHERTYPE = 0x88B5;

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <interface>\n";
        return 1;
    }

    const char* ifname = argv[1];

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

    struct sockaddr_ll addr{};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    std::cout << "Listening on " << ifname << " for EtherType 0x"
              << std::hex << MY_ETHERTYPE << std::dec << "\n";

    unsigned char buffer[2000];

    while (true) {
        ssize_t n = recv(sock, buffer, sizeof(buffer), 0);
        if (n <= 0) continue;

        if (n < 14) continue; // not even Ethernet header

        // parse MACs
        unsigned char* dst = buffer;
        unsigned char* src = buffer + 6;

        // EtherType already filtered by kernel — but double check
        uint16_t ethertype = (buffer[12] << 8) | buffer[13];
        if (ethertype != MY_ETHERTYPE) continue;

        std::cout << "Received frame from "
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
}
