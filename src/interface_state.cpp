#include <linux/if_packet.h>
#include <algorithm>
#include "interface_state.hpp"
#include "logger.hpp"

std::unordered_map<std::string, ActiveEthInterface> activeEthInterfaces;

// dicover all ethernet interfaces and get all related info of the interface
std::unordered_map<std::string, EthInterface> discoverEthInterfaces() {
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

        EthInterface& iface = ethInterfaces[ifname]; // trick - get or create interface entry
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
                iface.has_ipv4 = true;
                break;
            }

            // network layer - get ipv6 address
            case AF_INET6: {
                struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)ifa->ifa_addr;
                if (!IN6_IS_ADDR_LINKLOCAL(&addr_in6->sin6_addr)) { // skip link-local addresses (fe80::)
                    std::memcpy(iface.ipv6, addr_in6->sin6_addr.s6_addr, 16);
                    iface.has_ipv6 = true;
                }
                break;
            }

            default:
                // we do not need anything from other address families
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


// check if interface is UP and RUNNING
bool isInterfaceUp(const char* ifname) {
    int temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (temp_sock < 0) {
        LOG_ERROR("socket() failed: " << strerror(errno));
        return false;
    }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(temp_sock, SIOCGIFFLAGS, &ifr) < 0) {
        LOG_ERROR("ioctl() SIOCGIFFLAGS failed: " << strerror(errno));
        close(temp_sock);
        return false;
    }

    close(temp_sock);

    // check for UP and RUNNING flags
    return (ifr.ifr_flags & IFF_UP) && (ifr.ifr_flags & IFF_RUNNING);
}

void updateInterface(const EthInterface& ethInterface) {
    auto it = activeEthInterfaces.find(ethInterface.name);

    /* will ignore MAC change for now, will need to make that */

    ActiveEthInterface& activeIf = it->second;
    if (activeIf.ipv4 != ethInterface.ipv4 || std::memcmp(activeIf.ipv6, ethInterface.ipv6, 16) != 0) {
        LOG_INFO("IP address change detected on " << ethInterface.name);

        activeIf.ipv4 = ethInterface.ipv4;
        std::memcpy(activeIf.ipv6, ethInterface.ipv6, 16);

        buildEthernetFrame(activeIf.send_frame, activeIf.mac, activeIf.ipv4, activeIf.ipv6);
    }
}

// add new interface to monitoring
void addInterface(const EthInterface& ethInterface) {
    if (activeEthInterfaces.count(ethInterface.name) > 0) {
        updateInterface(ethInterface);
        return;
    }

    if (!isInterfaceUp(ethInterface.name.c_str())) {
        LOG_DEBUG("Interface " << ethInterface.name << " is not UP/RUNNING, skipping");
        return;
    }
    LOG_INFO("Adding interface: " << ethInterface.name);

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

    // create state
    ActiveEthInterface activeIfData;
    activeIfData.name = ethInterface.name;
    activeIfData.sockfd = sockfd;
    activeIfData.ifindex = ethInterface.ifindex;
    std::memcpy(activeIfData.mac, ethInterface.mac, MAC_ADDR_LEN);
    activeIfData.last_send_time = 0; // do we need this?
    // also need to check if is even there
    activeIfData.ipv4 = ethInterface.ipv4;
    std::memcpy(activeIfData.ipv6, ethInterface.ipv6, 16);

    // build frame
    buildEthernetFrame(activeIfData.send_frame, activeIfData.mac, activeIfData.ipv4, activeIfData.ipv6);

    // append to active interfaces
    activeEthInterfaces[ethInterface.name] = activeIfData;

    LOG_INFO("Monitoring interface: " << activeIfData.name << " MAC: ");
    debug::printMAC(activeIfData.mac);
}


// remove interface from monitoring
void removeInterface(const std::string& ifname) {
    auto it = activeEthInterfaces.find(ifname);
    if (it == activeEthInterfaces.end()) return;

    LOG_INFO("Removing interface: " << ifname);
    close(it->second.sockfd);
    activeEthInterfaces.erase(it);
}


// check for interface changes (called periodically)
void checkEthInterfaces() {
    std::unordered_map<std::string, EthInterface> allEthInterfaces = discoverEthInterfaces();

    // add new interfaces4
    for (const auto& [ifname, ethInterface] : allEthInterfaces) {
        addInterface(ethInterface);
    }

    // remove disappeared or down interfaces
    std::vector<std::string> to_remove;
    // to_remove.reserve(activeInterfaces.size()); // optional optimization
    for (const auto& [ifname, state] : activeEthInterfaces) {
        if (!isInterfaceUp(ifname.c_str())) {
            to_remove.push_back(ifname);
        }
    }

    for (const auto& ifname : to_remove) {
        removeInterface(ifname);
    }
}
