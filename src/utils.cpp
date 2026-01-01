#include "utils.hpp"

std::unordered_map<std::string, Neighbour> neighbors;

/* TEMPOERARY FOR DEBUGGING */
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

/* TEMPOERARY FOR DEBUGGING */
void printMACFromString(const std::string& mac_str) {
    const uint8_t* mac = reinterpret_cast<const uint8_t*>(mac_str.data());
    std::cout << std::hex << std::setfill('0')
              << std::setw(2) << (int)mac[0] << ":"
              << std::setw(2) << (int)mac[1] << ":"
              << std::setw(2) << (int)mac[2] << ":"
              << std::setw(2) << (int)mac[3] << ":"
              << std::setw(2) << (int)mac[4] << ":"
              << std::setw(2) << (int)mac[5]
              << std::dec;
}

std::string macToString(const uint8_t* mac) {
    return std::string(reinterpret_cast<const char*>(mac), MAC_ADDR_LEN);
}

void storeNeighbor(const uint8_t* buffer, ssize_t n, const char* ifname) {
    // do we even need to validate this stuff? my protocol is strict :)
    if (n < PAYLOAD_OFFSET + (ssize_t)sizeof(NeighborPayload)) {
        std::cerr << "Packet too small, ignoring\n";
        return;
    }

    const uint8_t* src_mac = buffer + MAC_ADDR_LEN;
    std::string mac_key = macToString(src_mac);

    const NeighborPayload* payload = reinterpret_cast<const NeighborPayload*>(buffer + PAYLOAD_OFFSET);

    // store or update neighbor info
    Neighbour neighbor{};
    std::memcpy(&neighbor.payload, payload, sizeof(NeighborPayload));
    std::strncpy(neighbor.ifName, ifname, IFNAMSIZ);
    neighbor.lastSeen = time(nullptr);

    neighbors[mac_key] = neighbor;

    // std::cout << "Stored/Updated neighbor with MAC: ";
    // printMAC(src_mac);
    // std::cout << "\n";

    std::cout << "Current neighbors:\n";
    for (const auto& [mac_str, neighbor] : neighbors) {
        std::cout << "MAC: ";
        printMACFromString(mac_str);  // ← convert binary to hex
        std::cout << " on " << neighbor.ifName << "\n";
    }

}

void printFrameData(const uint8_t* buffer, ssize_t n) {
    // should i filter out packets where size != 34 bytes?
    // std::cout << "Received packet of size: " << n << "\n";

    #define DST_MAC_ADDR_OFFSET 6 // need to clean this up later, disgusting
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

    // for test purposes, hardcoded src MAC
    // const uint8_t hardcodedSrcMac[MAC_ADDR_LEN] = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};
    // std::memcpy(frame + MAC_ADDR_LEN, hardcodedSrcMac, MAC_ADDR_LEN);

    frame[ETH_TYPE_OFFSET] = (ETH_P_NEIGHBOR_DISC >> 8) & 0xff;
    frame[ETH_TYPE_OFFSET + 1] = (ETH_P_NEIGHBOR_DISC) & 0xff;

    // dummy IP data for now
    NeighborPayload hostData{};
    hostData.ipv4 = htonl(12345);
    hostData.ipv6[0] = 0x66;

    std::memcpy(frame + PAYLOAD_OFFSET, &hostData, sizeof(hostData));
}