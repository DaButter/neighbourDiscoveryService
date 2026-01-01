#include "utils.hpp"

// TEMPORARY - DEBUG SRC MAC ADDR
void printMAC(const uint8_t* mac) {
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

void printFrameData(const uint8_t* buffer, ssize_t n) {
    // should i filter out packets where size != 34 bytes?
    std::cout << "Received packet of size: " << n << "\n";

    #define DST_MAC_ADDR_OFFSET 6 // need to clean this up later
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

    const NeighborPayload* payload = reinterpret_cast<const NeighborPayload*>(buffer + PAYLOAD_OFFSET);
    uint32_t ipv4 = ntohl(payload->ipv4);
    uint8_t* ipv6 = const_cast<uint8_t*>(payload->ipv6);

    std::cout << "Received from IPv4: " << ipv4 << " IPv6: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ipv6[i];
        if (i % 2 == 1 && i != 15) std::cout << ":";
    }
    std::cout << std::dec << "\n";
}

void buildEthernetFrame(uint8_t* frame, const uint8_t* srcMac, const uint8_t* dstMac) {
    std::memcpy(frame, dstMac, MAC_ADDR_LEN);                 // Destination MAC
    std::memcpy(frame + MAC_ADDR_LEN, srcMac, MAC_ADDR_LEN);  // Source MAC

    frame[ETH_TYPE_OFFSET] = (ETH_P_NEIGHBOR_DISC >> 8) & 0xff;
    frame[ETH_TYPE_OFFSET + 1] = (ETH_P_NEIGHBOR_DISC) & 0xff;

    // dummy IP data for now
    NeighborPayload hostData{};
    hostData.ipv4 = htonl(12345);
    hostData.ipv6[0] = 0x66;

    std::memcpy(frame + PAYLOAD_OFFSET, &hostData, sizeof(hostData));
}