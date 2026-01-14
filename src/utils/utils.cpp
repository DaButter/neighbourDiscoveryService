#include "utils.hpp"

namespace frame {
    // build Ethernet frame: [Dst MAC (6)] [Src MAC (6)] [EtherType (2)] [Payload (52)]
    void build(uint8_t* frame, const uint8_t* srcMac, const uint8_t* machineId, const uint32_t& ipv4, const uint8_t* ipv6) {
        std::memcpy(frame, broadcastMac, MAC_ADDR_LEN);
        std::memcpy(frame + MAC_ADDR_LEN, srcMac, MAC_ADDR_LEN);

        uint16_t ethertype = htons(ETH_P_NEIGHBOR_DISC);
        std::memcpy(frame + ETH_TYPE_OFFSET, &ethertype, sizeof(ethertype));

        NeighborPayload hostData{};
        std::memcpy(hostData.machineId, machineId, MACHINE_ID_LEN);
        hostData.ipv4 = htonl(ipv4);
        std::memcpy(hostData.ipv6, ipv6, sizeof(hostData.ipv6));

        std::memcpy(frame + PAYLOAD_OFFSET, &hostData, sizeof(hostData));
    }
}

namespace utils {
    bool getMachineId(uint8_t* output) {
        std::ifstream file("/etc/machine-id");
        if (!file.is_open()) {
            LOG_ERROR("Failed to open /etc/machine-id");
            return false;
        }

        std::string machineId;
        std::getline(file, machineId);

        if (machineId.length() != MACHINE_ID_LEN) {
            LOG_ERROR("Invalid machine ID length: " << machineId.length());
            return false;
        }

        std::memcpy(output, machineId.data(), MACHINE_ID_LEN);
        return true;
    }

    void printIPv4(uint32_t ipv4) {
        if (ipv4 == 0) {
            std::cout << "none";
        } else {
            struct in_addr addr;
            addr.s_addr = ipv4;
            std::cout << inet_ntoa(addr);
        }
    }

    void printIPv6(const uint8_t* ipv6) {
        bool has_ipv6 = false;
        for (int i = 0; i < 16; ++i) {
            if (ipv6[i] != 0) {
                has_ipv6 = true;
                break;
            }
        }

        if (!has_ipv6) {
            std::cout << "none";
        } else {
            for (int i = 0; i < 16; i += 2) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ipv6[i]
                          << std::setw(2) << (int)ipv6[i+1];
                if (i < 14) std::cout << ":";
            }
            std::cout << std::dec;
        }
    }

    void printMAC(const uint8_t* mac) {
        std::cout << std::hex << std::setfill('0')
                  << std::setw(2) << (int)mac[0] << ":"
                  << std::setw(2) << (int)mac[1] << ":"
                  << std::setw(2) << (int)mac[2] << ":"
                  << std::setw(2) << (int)mac[3] << ":"
                  << std::setw(2) << (int)mac[4] << ":"
                  << std::setw(2) << (int)mac[5]
                  << std::dec;
    }

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
}
