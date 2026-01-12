#pragma once

#include <iostream>
#include <sys/time.h>
#include <iomanip>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include "common.hpp"

namespace utils {
    std::string getTimestamp();
    bool getMachineId(uint8_t* output);
}

#define LOG_DEBUG(msg) \
    std::cout << "|DEBUG " << utils::getTimestamp() << "| " << msg << std::endl

#define LOG_WARN(msg) \
    std::cout << "|WARN  " << utils::getTimestamp() << "| " << msg << std::endl

#define LOG_INFO(msg) \
    std::cout << "|INFO  " << utils::getTimestamp() << "| " << msg << std::endl

#define LOG_ERROR(msg) \
    std::cerr << "|ERROR " << utils::getTimestamp() << "| " << msg << std::endl

namespace debug {
    void printMAC(const uint8_t* mac);
    void printMACFromString(const std::string& macStr);
    void printFrameData(const uint8_t* buffer);
    std::string macToString(const uint8_t* mac);
    std::string machineIdToString(const uint8_t* machineId);
}

namespace frame {
    void build(uint8_t* frame, const uint8_t* srcMac, const uint8_t* machineId, const uint32_t& ipv4, const uint8_t* ipv6);
}