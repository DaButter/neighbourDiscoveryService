#include "utils.hpp"

// Helper for 64-bit network byte order
#ifndef htonll
uint64_t htonll(uint64_t value) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    #else
        return value;
    #endif
}
#define ntohll htonll
#endif

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
    // should i filter out packets where size != 32?
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
    uint64_t timestamp = ntohll(payload->timestamp);
    time_t recv_time = static_cast<time_t>(timestamp);

    std::cout << "Received from IP: " << ipv4 << " at timestamp: " << recv_time << "\n";
}

void buildEthernetFrame(uint8_t* frame, const uint8_t* src_mac, const uint8_t* dst_mac) {
    std::memcpy(frame, dst_mac, MAC_ADDR_LEN);                 // Destination MAC
    std::memcpy(frame + MAC_ADDR_LEN, src_mac, MAC_ADDR_LEN);  // Source MAC

    frame[ETH_TYPE_OFFSET] = (ETH_P_NEIGHBOR_DISC >> 8) & 0xff;
    frame[ETH_TYPE_OFFSET + 1] = (ETH_P_NEIGHBOR_DISC) & 0xff;

    NeighborPayload hostData;
    // std::memcpy(hostData.mac, src_mac, MAC_ADDR_LEN);
    hostData.ipv4 = htonl(12345);
    hostData.timestamp = htonll(static_cast<uint64_t>(time(nullptr)));

    std::memcpy(frame + PAYLOAD_OFFSET, &hostData, sizeof(hostData));
}