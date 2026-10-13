#pragma once

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <sys/types.h>
#include <vector>
#include <span>

class GtpuSocket {
    int fd_;
    sockaddr_in gnb_{};

public:
    GtpuSocket();
    ~GtpuSocket();

    GtpuSocket(const GtpuSocket&) = delete;
    GtpuSocket& operator=(const GtpuSocket&) = delete;

    ssize_t send_to_gnb(std::span<const uint8_t> packet);
    ssize_t receive(std::span<uint8_t> buffer, sockaddr_in& sender);

    int fd() const;
};
