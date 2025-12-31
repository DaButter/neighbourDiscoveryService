#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>

#define PAYLOAD_OFFSET 14
#define MAC_ADDR_LEN 6

static constexpr uint16_t ETH_P_NEIGHBOR_DISC = 0x88B5;

void printMAC(const uint8_t* mac);

void printBuffer(const uint8_t* buffer,
                 ssize_t n);

void buildEthernetFrame(uint8_t* frame,
                        const uint8_t* src_mac,
                        const uint8_t* dst_mac);