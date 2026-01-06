#pragma once

#include <iostream>
#include <sys/time.h>
#include <iomanip>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "common.hpp"

std::string getTimestamp();

#define LOG_DEBUG(msg) \
    std::cout << "|DEBUG " << getTimestamp() << "| " << msg << std::endl

#define LOG_INFO(msg) \
    std::cout << "|INFO  " << getTimestamp() << "| " << msg << std::endl

#define LOG_ERROR(msg) \
    std::cerr << "|ERROR " << getTimestamp() << "| " << msg << std::endl

namespace debug {
    void printMAC(const uint8_t* mac);
    void printMACFromString(const std::string& macStr);
    void printFrameData(const uint8_t* buffer);
    std::string macToString(const uint8_t* mac);
}

namespace frame {
    void build(uint8_t* frame, const uint8_t* srcMac, const uint32_t& ipv4, const uint8_t* ipv6);
}