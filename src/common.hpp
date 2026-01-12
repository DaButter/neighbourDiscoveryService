#pragma once

#include <cstdint>

inline constexpr std::size_t MAC_ADDR_LEN = 6;
inline constexpr std::size_t ETH_TYPE_OFFSET = 12;
inline constexpr std::size_t PAYLOAD_OFFSET = 14;
inline constexpr std::size_t MACHINE_ID_LEN = 32;
inline constexpr int NEIGHBOR_EXPIRY_SECONDS = 30;
inline constexpr uint8_t broadcastMac[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
    IEEE Std 802 - Local Experimental Ethertype
    https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml
*/
inline constexpr uint16_t ETH_P_NEIGHBOR_DISC = 0x88B5;

struct NeighborPayload {
    uint8_t machineId[MACHINE_ID_LEN];
    uint32_t ipv4;
    uint8_t ipv6[16];
} __attribute__((packed));

static_assert(sizeof(NeighborPayload) == 52, "NeighborPayload must be 52 bytes");
