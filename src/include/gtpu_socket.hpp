#pragma once

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <sys/types.h>
#include <vector>

class GtpuSocket {
    int fd_;
    sockaddr_in gnb_{};

public:
    GtpuSocket();
    ~GtpuSocket();

    GtpuSocket(const GtpuSocket&) = delete;
    GtpuSocket& operator=(const GtpuSocket&) = delete;

    ssize_t send_to_gnb(const std::vector<uint8_t>& packet);
    ssize_t receive(uint8_t* buffer, size_t buffer_size, sockaddr_in& sender);

    int fd() const;
};
