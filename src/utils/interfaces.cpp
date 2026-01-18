#include "interfaces.hpp"

namespace interfaces {
    std::unordered_map<std::string, MonitoredEthInterface> monitoredEthInterfaces;

    static const std::unordered_map<std::string, EthInterface>& discover() {
        static std::unordered_map<std::string, EthInterface> discoveredInterfaces;
        discoveredInterfaces.clear();

        struct ifaddrs* ifaddr;

        if (getifaddrs(&ifaddr) == -1) {
            LOG_ERROR("getifaddrs() failed: " << strerror(errno));
            return discoveredInterfaces;
        }

        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            /* skip entries without valid address, loopback, or down interfaces */
            if (ifa->ifa_addr == nullptr) continue;
            if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_RUNNING)) continue;

            std::string ifname = ifa->ifa_name;
            if (ifname == "lo") continue;

            EthInterface& iface = discoveredInterfaces[ifname];
            iface.name = ifname;

            switch (ifa->ifa_addr->sa_family) {
                /* link layer - get MAC address and interface index */
                case AF_PACKET: {
                    struct sockaddr_ll* addr_ll = (struct sockaddr_ll*)ifa->ifa_addr;
                    iface.ifindex = addr_ll->sll_ifindex;
                    if (addr_ll->sll_halen == MAC_ADDR_LEN) {
                        std::memcpy(iface.mac, addr_ll->sll_addr, MAC_ADDR_LEN);
                    }
                    break;
                }

                /* network layer - get ipv4 address */
                case AF_INET: {
                    if (iface.hasIPv4()) break;
                    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                    iface.ipv4 = addr_in->sin_addr.s_addr;
                    break;
                }

                /* network layer - get ipv6 address */
                case AF_INET6: {
                    if (iface.hasIPv6()) break;
                    struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)ifa->ifa_addr;
                    std::memcpy(iface.ipv6, addr_in6->sin6_addr.s6_addr, 16);
                    break;
                }

                default:
                    break;
            }
        }

        freeifaddrs(ifaddr);

        // in case interface has no link layer data, remove it
        for (auto it = discoveredInterfaces.begin(); it != discoveredInterfaces.end(); ) {
            if (it->second.ifindex == 0) {
                it = discoveredInterfaces.erase(it);
            } else {
                ++it;
            }
        }

        return discoveredInterfaces;
    }

    static int createAndBindSocket(const EthInterface& ethInterface) {
        int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR_DISC));
        if (sockfd < 0) {
            LOG_ERROR("Failed to create socket for " << ethInterface.name << ": " << strerror(errno));
            return -1;
        }

        /* by default, Linux kernel allocated small recv buffer: 212992 bytes (able to queue ~3000 incoming NeigborPayload(66 bytes without padding) packets)
           so the packets do not get dropped, when there are many neighbors,
           lets increase recv buffer 8 MB (holds ~130,000 packets of 66 bytes)
        */
        int bufsize = 8 * 1024 * 1024;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0) {
            LOG_WARN("Failed to increase recv buffer on " << ethInterface.name);
        }

        struct sockaddr_ll addr{};
        addr.sll_family = AF_PACKET;
        addr.sll_ifindex = ethInterface.ifindex;
        addr.sll_protocol = htons(ETH_P_NEIGHBOR_DISC);

        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR("Failed to bind to " << ethInterface.name << ": " << strerror(errno));
            close(sockfd);
            return -1;
        }

        return sockfd;
    }

    static void addOrUpdate(const EthInterface& ethInterface, const uint8_t* machineId) {
        auto it = monitoredEthInterfaces.find(ethInterface.name);

        if (it != monitoredEthInterfaces.end()) {
            // update existing interface
            MonitoredEthInterface& monitoredIf = it->second;

            bool ifindex_changed = (monitoredIf.ifData.ifindex != ethInterface.ifindex);
            bool mac_changed = (std::memcmp(monitoredIf.ifData.mac, ethInterface.mac, MAC_ADDR_LEN) != 0);
            bool ipv4_changed = (monitoredIf.ifData.ipv4 != ethInterface.ipv4);
            bool ipv6_changed = (std::memcmp(monitoredIf.ifData.ipv6, ethInterface.ipv6, 16) != 0);

            if (ifindex_changed || mac_changed || ipv4_changed || ipv6_changed) {
                if (ifindex_changed || mac_changed) {
                    LOG_WARN("Critical interface change on " << ethInterface.name << " - recreating socket");
                    close(monitoredIf.sockfd);

                    int sockfd = createAndBindSocket(ethInterface);
                    if (sockfd < 0) {
                        monitoredEthInterfaces.erase(it);
                        return;
                    }

                    monitoredIf.sockfd = sockfd;
                    monitoredIf.ifData.ifindex = ethInterface.ifindex;
                    std::memcpy(monitoredIf.ifData.mac, ethInterface.mac, MAC_ADDR_LEN);
                }

                if (ipv4_changed) monitoredIf.ifData.ipv4 = ethInterface.ipv4;
                if (ipv6_changed) std::memcpy(monitoredIf.ifData.ipv6, ethInterface.ipv6, 16);

                frame::build(monitoredIf.send_frame, monitoredIf.ifData.mac, machineId,
                             monitoredIf.ifData.ipv4, monitoredIf.ifData.ipv6);
            }
        } else {
            // add new interface
            int sockfd = createAndBindSocket(ethInterface);
            if (sockfd < 0) return;

            MonitoredEthInterface monitoredIfData{};
            monitoredIfData.sockfd = sockfd;
            monitoredIfData.ifData.name = ethInterface.name;
            monitoredIfData.ifData.ifindex = ethInterface.ifindex;
            std::memcpy(monitoredIfData.ifData.mac, ethInterface.mac, MAC_ADDR_LEN);

            if (ethInterface.hasIPv4()) monitoredIfData.ifData.ipv4 = ethInterface.ipv4;
            if (ethInterface.hasIPv6()) std::memcpy(monitoredIfData.ifData.ipv6, ethInterface.ipv6, 16);

            frame::build(monitoredIfData.send_frame, monitoredIfData.ifData.mac, machineId,
                         monitoredIfData.ifData.ipv4, monitoredIfData.ifData.ipv6);

            monitoredIfData.send_addr.sll_family = AF_PACKET;
            monitoredIfData.send_addr.sll_ifindex = ethInterface.ifindex;
            monitoredIfData.send_addr.sll_halen = MAC_ADDR_LEN;
            std::memcpy(monitoredIfData.send_addr.sll_addr, broadcastMac, MAC_ADDR_LEN);

            monitoredEthInterfaces[ethInterface.name] = monitoredIfData;
            LOG_INFO("Added interface: " << ethInterface.name);
        }
    }

    void checkAndUpdate(const uint8_t* machineId) {
        const auto& allEthInterfaces = discover();

        // add new interfaces or update existing ones
        for (const auto& [ifname, ethInterface] : allEthInterfaces) {
            addOrUpdate(ethInterface, machineId);
        }

        // remove interfaces no longer present
        for (auto it = monitoredEthInterfaces.begin(); it != monitoredEthInterfaces.end(); ) {
            if (allEthInterfaces.find(it->first) == allEthInterfaces.end()) {
                LOG_INFO("Removing interface: " << it->first);
                close(it->second.sockfd);
                it = monitoredEthInterfaces.erase(it);
            } else {
                ++it;
            }
        }
    }

}
