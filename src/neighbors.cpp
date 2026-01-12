#include "neighbors.hpp"

namespace neighbor {
    std::unordered_map<std::string, Neighbor> neighbors;

    // check if we can optimize this
    void store(const uint8_t* buffer, const char* ifname) {
        const uint8_t* srcMac = buffer + MAC_ADDR_LEN;
        const NeighborPayload* payload = reinterpret_cast<const NeighborPayload*>(buffer + PAYLOAD_OFFSET);

        // esxtract device ID from payload (32 hex chars)
        // std::string machineId(payload->machineId, MACHINE_ID_LEN);
        std::string machineId(reinterpret_cast<const char*>(payload->machineId), MACHINE_ID_LEN);

        // get or create neighbor entry
        Neighbor& neighbor = neighbors[machineId];
        neighbor.machineId = machineId;

        // update or create connection for this interface
        Connection& conn = neighbor.connections[ifname];
        std::strncpy(conn.localIfName, ifname, IFNAMSIZ);
        std::memcpy(conn.remoteMac, srcMac, MAC_ADDR_LEN);
        conn.remoteIpv4 = ntohl(payload->ipv4);
        std::memcpy(conn.remoteIpv6, payload->ipv6, 16);
        conn.lastSeen = time(nullptr);

        LOG_DEBUG("Updated neighbor " << machineId << " on " << ifname);
        LOG_DEBUG("  Total neighbors: " << neighbors.size());
        LOG_DEBUG("  Connections for this neighbor: " << neighbor.connections.size());
    }

    // check if this can be optimized further
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
