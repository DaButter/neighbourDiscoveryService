#include "utils.hpp"

std::string getTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    time_t now = tv.tv_sec;
    struct tm* tm_info = localtime(&now);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%s.%06ld", buffer, tv.tv_usec);

    return std::string(timestamp);
}

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

    std::string macToString(const uint8_t* mac) {
        return std::string(reinterpret_cast<const char*>(mac), MAC_ADDR_LEN);
    }
}

namespace frame {
    void build(uint8_t* frame, const uint8_t* srcMac, const uint32_t& ipv4, const uint8_t* ipv6) {

        std::memcpy(frame, broadcastMac, MAC_ADDR_LEN);           // destination MAC
        std::memcpy(frame + MAC_ADDR_LEN, srcMac, MAC_ADDR_LEN);  // source MAC

        frame[ETH_TYPE_OFFSET] = (ETH_P_NEIGHBOR_DISC >> 8) & 0xff;
        frame[ETH_TYPE_OFFSET + 1] = (ETH_P_NEIGHBOR_DISC) & 0xff;

        NeighborPayload hostData{};
        hostData.ipv4 = ipv4;
        std::memcpy(hostData.ipv6, ipv6, sizeof(hostData.ipv6));

        std::memcpy(frame + PAYLOAD_OFFSET, &hostData, sizeof(hostData));
    }
}