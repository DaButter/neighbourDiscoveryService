#include "neighbors.hpp"

namespace neighbor {
    std::unordered_map<std::string, Neighbor> neighbors;

    void store(const uint8_t* buffer, const char* ifname) {
        // do we even need to validate this stuff? my protocol is strict :)
        // if (n < PAYLOAD_OFFSET + (ssize_t)sizeof(NeighborPayload)) {
            // LOG_ERROR("Packet too small, ignoring");
            // return;
        // }

        const uint8_t* src_mac = buffer + MAC_ADDR_LEN;
        std::string mac_key = debug::macToString(src_mac);

        const NeighborPayload* payload = reinterpret_cast<const NeighborPayload*>(buffer + PAYLOAD_OFFSET);

        Neighbor neighbor{};
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

    void checkTimeout(const time_t& now) {
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

}
