#pragma once

#include <iostream>
#include <string>

std::string getTimestamp();

#define LOG_DEBUG(msg) \
    std::cout << "|DEBUG " << getTimestamp() << "| " << msg << std::endl

#define LOG_INFO(msg) \
    std::cout << "|INFO  " << getTimestamp() << "| " << msg << std::endl

#define LOG_ERROR(msg) \
    std::cerr << "|ERROR " << getTimestamp() << "| " << msg << std::endl
