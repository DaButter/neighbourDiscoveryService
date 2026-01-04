#include "logger.hpp"
#include <iomanip>
#include <ctime>
#include <sys/time.h>

std::string getTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    time_t now = tv.tv_sec;
    struct tm* tm_info = localtime(&now);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%s.%06ld", buffer, tv.tv_usec);

    return std::string(timestamp);
}