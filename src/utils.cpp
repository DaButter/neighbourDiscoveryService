#include "utils.hpp"
#include "logger.hpp"

std::unordered_map<std::string, Neighbour> neighbors;

/* TEMPORARY FOR DEBUGGING , will use for CLI later */
namespace debug {
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

    void printMACFromString(const std::string& macStr) {
        const uint8_t* mac = reinterpret_cast<const uint8_t*>(macStr.data());
        std::cout << std::hex << std::setfill('0')
                  << std::setw(2) << (int)mac[0] << ":"
                  << std::setw(2) << (int)mac[1] << ":"
                  << std::setw(2) << (int)mac[2] << ":"
                  << std::setw(2) << (int)mac[3] << ":"
                  << std::setw(2) << (int)mac[4] << ":"
                  << std::setw(2) << (int)mac[5]
                  << std::dec;
    }

    void printFrameData(const uint8_t* buffer) {
        // src MAC is at offset 6
        std::cout << "Received packet from "
                  << std::hex
                  << std::setw(2) << std::setfill('0')
                  << (int)buffer[0+MAC_ADDR_LEN] << ":"
                  << (int)buffer[1+MAC_ADDR_LEN] << ":"
                  << (int)buffer[2+MAC_ADDR_LEN] << ":"
                  << (int)buffer[3+MAC_ADDR_LEN] << ":"
                  << (int)buffer[4+MAC_ADDR_LEN] << ":"
                  << (int)buffer[5+MAC_ADDR_LEN]
                  << std::dec
                  << "  payload: ";

        const NeighborPayload* payload = reinterpret_cast<const NeighborPayload*>(buffer + PAYLOAD_OFFSET);

        /* print IPv4 */
        struct in_addr ipv4_addr;
        ipv4_addr.s_addr = payload->ipv4;
        std::cout << "Received from IPv4: " << inet_ntoa(ipv4_addr) << " IPv6: ";

        /* print IPv6 */
        uint8_t* ipv6 = const_cast<uint8_t*>(payload->ipv6);
        for (int i = 0; i < 16; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ipv6[i];
            if (i % 2 == 1 && i != 15) std::cout << ":";
        }
        std::cout << std::dec << "\n";
    }
}

std::string macToString(const uint8_t* mac) {
    return std::string(reinterpret_cast<const char*>(mac), MAC_ADDR_LEN);
}

void storeNeighbor(const uint8_t* buffer, ssize_t n, const char* ifname) {
    // do we even need to validate this stuff? my protocol is strict :)
    if (n < PAYLOAD_OFFSET + (ssize_t)sizeof(NeighborPayload)) {
        LOG_ERROR("Packet too small, ignoring");
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

    LOG_DEBUG("Current neighbors:");
    for (const auto& [mac_str, neighbor] : neighbors) {
        std::cout << "MAC: ";
        debug::printMACFromString(mac_str);  // convert binary to hex
        std::cout << " on " << neighbor.ifName << "\n";
    }
}

void timeoutNeighbors() {
    time_t now = time(nullptr);
    for (auto it = neighbors.begin(); it != neighbors.end(); ) {
        if (now - it->second.lastSeen > NEIGHBOR_EXPIRY_SECONDS) {
            LOG_DEBUG("Neighbor timed out: ");
            debug::printMACFromString(it->first);
            std::cout << "\n";
            it = neighbors.erase(it);
        } else {
            ++it;
        }
    }
}

void buildEthernetFrame(uint8_t* frame, const uint8_t* srcMac, uint32_t ipv4, const uint8_t* ipv6) {

    std::memcpy(frame, broadcastMac, MAC_ADDR_LEN);           // destination MAC
    std::memcpy(frame + MAC_ADDR_LEN, srcMac, MAC_ADDR_LEN);  // source MAC

    // TMP: for test purposes, hardcoded src MAC
    // const uint8_t hardcodedSrcMac[MAC_ADDR_LEN] = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};
    // std::memcpy(frame + MAC_ADDR_LEN, hardcodedSrcMac, MAC_ADDR_LEN);

    frame[ETH_TYPE_OFFSET] = (ETH_P_NEIGHBOR_DISC >> 8) & 0xff;
    frame[ETH_TYPE_OFFSET + 1] = (ETH_P_NEIGHBOR_DISC) & 0xff;

    NeighborPayload hostData{};
    hostData.ipv4 = ipv4;
    std::memcpy(hostData.ipv6, ipv6, sizeof(hostData.ipv6));

    std::memcpy(frame + PAYLOAD_OFFSET, &hostData, sizeof(hostData));
}
