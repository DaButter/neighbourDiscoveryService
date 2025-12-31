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

void printBuffer(const uint8_t* buffer, ssize_t n) {
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

void buildEthernetFrame(uint8_t* frame, const uint8_t* src_mac, const uint8_t* dst_mac) {
    const char* payload = "HELLO_FROM_VM1_AUSTRIS";

    std::memcpy(frame, dst_mac, MAC_ADDR_LEN);                 // Destination MAC
    std::memcpy(frame + MAC_ADDR_LEN, src_mac, MAC_ADDR_LEN);  // Source MAC

    frame[12] = (ETH_P_NEIGHBOR_DISC >> 8) & 0xff;
    frame[13] = (ETH_P_NEIGHBOR_DISC) & 0xff;

    std::memcpy(frame + PAYLOAD_OFFSET, payload, std::strlen(payload));

}