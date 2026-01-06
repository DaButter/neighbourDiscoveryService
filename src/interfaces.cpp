#include "interfaces.hpp"

namespace interfaces {
    std::unordered_map<std::string, ActiveEthInterface> activeEthInterfaces;


    std::unordered_map<std::string, EthInterface> discover() {
        std::unordered_map<std::string, EthInterface> ethInterfaces;
        struct ifaddrs* ifaddr;

        if (getifaddrs(&ifaddr) == -1) {
            LOG_ERROR("getifaddrs() failed: " << strerror(errno));
            return ethInterfaces;
        }

        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;

            std::string ifname = ifa->ifa_name;
            if (ifname == "lo") continue;

            if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_RUNNING)) {
                continue;
            }

            EthInterface& iface = ethInterfaces[ifname];
            iface.name = ifname;

            switch (ifa->ifa_addr->sa_family) {
                // link layer - get MAC address and interface index
                case AF_PACKET: {
                    struct sockaddr_ll* addr_ll = (struct sockaddr_ll*)ifa->ifa_addr;
                    iface.ifindex = addr_ll->sll_ifindex;
                    if (addr_ll->sll_halen == MAC_ADDR_LEN) {
                        std::memcpy(iface.mac, addr_ll->sll_addr, MAC_ADDR_LEN);
                    }
                    break;
                }

                // network layer - get ipv4 address
                case AF_INET: {
                    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                    iface.ipv4 = addr_in->sin_addr.s_addr;
                    break;
                }

                // network layer - get ipv6 address
                case AF_INET6: {
                    struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)ifa->ifa_addr;
                    if (!IN6_IS_ADDR_LINKLOCAL(&addr_in6->sin6_addr)) {
                        std::memcpy(iface.ipv6, addr_in6->sin6_addr.s6_addr, 16);
                    }
                    break;
                }

                default:
                    break;
            }
        }

        freeifaddrs(ifaddr);

        // in case we got interfaces with no AF_PACKET for some reason (virtual interfaces?) - remove them
        for (auto it = ethInterfaces.begin(); it != ethInterfaces.end(); ) {
            if (it->second.ifindex == 0) {
                it = ethInterfaces.erase(it);
            } else {
                ++it;
            }
        }

        return ethInterfaces;
    }

    void update(const EthInterface& ethInterface) {
        /* will ignore MAC change for now, will need to implement that */

        ActiveEthInterface& activeIf = activeEthInterfaces.find(ethInterface.name)->second;

        if (activeIf.ifData.ipv4 != ethInterface.ipv4 || std::memcmp(activeIf.ifData.ipv6, ethInterface.ipv6, 16) != 0) {
            LOG_INFO("IP address change detected on " << ethInterface.name);

            activeIf.ifData.ipv4 = ethInterface.ipv4;
            std::memcpy(activeIf.ifData.ipv6, ethInterface.ipv6, 16);
            frame::build(activeIf.send_frame, activeIf.ifData.mac, activeIf.ifData.ipv4, activeIf.ifData.ipv6);
        }
    }

    void add(const EthInterface& ethInterface) {
        // check if interface already monitored
        if (activeEthInterfaces.count(ethInterface.name) > 0) {
            update(ethInterface);
            return;
        }

        LOG_INFO("Adding new interface: " << ethInterface.name);

        // create socket
        int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR_DISC));
        if (sockfd < 0) {
            LOG_ERROR("Failed to create socket for " << ethInterface.name << ": " << strerror(errno));
            return;
        }

        // bind to interface
        struct sockaddr_ll addr{};
        addr.sll_family = AF_PACKET;
        addr.sll_ifindex = ethInterface.ifindex;
        addr.sll_protocol = htons(ETH_P_NEIGHBOR_DISC);

        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR("Failed to bind to " << ethInterface.name << ": " << strerror(errno));
            close(sockfd);
            return;
        }

        // create active interface entry
        ActiveEthInterface activeIfData;
        activeIfData.ifData.name = ethInterface.name;
        activeIfData.sockfd = sockfd;
        activeIfData.ifData.ifindex = ethInterface.ifindex;
        std::memcpy(activeIfData.ifData.mac, ethInterface.mac, MAC_ADDR_LEN);

        if (ethInterface.hasIPv4())  activeIfData.ifData.ipv4 = ethInterface.ipv4;
        if (ethInterface.hasIPv6())  std::memcpy(activeIfData.ifData.ipv6, ethInterface.ipv6, 16);

        // build ethernet frame
        frame::build(activeIfData.send_frame, activeIfData.ifData.mac, activeIfData.ifData.ipv4, activeIfData.ifData.ipv6);

        // prebuild send address
        activeIfData.send_addr.sll_family = AF_PACKET;
        activeIfData.send_addr.sll_ifindex = ethInterface.ifindex;
        activeIfData.send_addr.sll_halen = MAC_ADDR_LEN;
        std::memcpy(activeIfData.send_addr.sll_addr, broadcastMac, MAC_ADDR_LEN);

        // append to active interfaces map
        activeEthInterfaces[activeIfData.ifData.name] = activeIfData;

        LOG_INFO("Monitoring interface: " << activeIfData.ifData.name << " MAC: ");
        debug::printMAC(activeIfData.ifData.mac);
    }

    void checkAndUpdate() {
        std::unordered_map<std::string, EthInterface> allEthInterfaces = discover();

        // add new interfaces or update existing ones
        for (const auto& [ifname, ethInterface] : allEthInterfaces) {
            add(ethInterface);
        }

        // remove interfaces no longer present (down or disappeared)
        for (auto it = activeEthInterfaces.begin(); it != activeEthInterfaces.end(); ) {
            if (allEthInterfaces.find(it->first) == allEthInterfaces.end()) {
                LOG_INFO("Removing interface: " << it->first);
                close(it->second.sockfd);
                it = activeEthInterfaces.erase(it);
            } else {
                ++it;
            }
        }
    }

}
