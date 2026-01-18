#include "neighbors.hpp"

namespace neighbor {
    std::unordered_map<std::string, Neighbor> neighbors;

    // preallocate for 10,000 neighbors to avoid rehashing
    void init() {
        neighbors.reserve(10000);
    }

    void store(const uint8_t* buffer, const char* ifname) {
        const uint8_t* srcMac = buffer + MAC_ADDR_LEN;
        const NeighborPayload* payload = reinterpret_cast<const NeighborPayload*>(buffer + PAYLOAD_OFFSET);
        std::string machineId(reinterpret_cast<const char*>(payload->machineId), MACHINE_ID_LEN);

        auto [it, inserted] = neighbors.try_emplace(machineId);
        Neighbor& neighbor = it->second;

        if (inserted) {
            neighbor.machineId = machineId;
        }

        // update or create connection entry
        auto [it_conn, inserted_conn] = neighbor.connections.try_emplace(ifname);
        Connection& conn = it_conn->second;

        if (inserted_conn) {
            std::snprintf(conn.localIfName, IFNAMSIZ, "%s", ifname);
        }
        std::memcpy(conn.remoteMac, srcMac, MAC_ADDR_LEN);
        conn.remoteIpv4 = ntohl(payload->ipv4);
        std::memcpy(conn.remoteIpv6, payload->ipv6, 16);
        conn.lastSeen = time(nullptr);
    }

    void checkTimeout(const time_t& now) {
        for (auto neighbor_it = neighbors.begin(); neighbor_it != neighbors.end(); ) {
            Neighbor& neighbor = neighbor_it->second;

            for (auto conn_it = neighbor.connections.begin(); conn_it != neighbor.connections.end(); ) {
                if (now - conn_it->second.lastSeen > NEIGHBOR_EXPIRY_SECONDS) {
                    LOG_DEBUG("Connection timeout: " << neighbor_it->first << " on " << conn_it->first);
                    conn_it = neighbor.connections.erase(conn_it);
                } else {
                    ++conn_it;
                }
            }

            if (neighbor.connections.empty()) {
                LOG_DEBUG("Neighbor removed, no active connections: " << neighbor_it->first);
                neighbor_it = neighbors.erase(neighbor_it);
            } else {
                ++neighbor_it;
            }
        }
    }

}
