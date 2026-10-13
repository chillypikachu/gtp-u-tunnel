#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <span>

class RawSocket {
    int fd_;
    std::array<uint8_t, 6> mac_address_;

public:
    explicit RawSocket(const char* name);
    ~RawSocket();

    RawSocket(const RawSocket&) = delete;
    RawSocket& operator=(const RawSocket&) = delete;

    ssize_t receive(std::span<uint8_t> buffer);
    ssize_t send_to(std::span<const uint8_t> buffer);

    const std::array<uint8_t, 6>& mac() const;
    int fd() const;
};
