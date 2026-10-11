#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>

class RawSocket {
    int fd_;
    std::array<uint8_t, 6> mac_address_;

public:
    explicit RawSocket(const char* name);
    ~RawSocket();

    RawSocket(const RawSocket&) = delete;
    RawSocket& operator=(const RawSocket&) = delete;

    ssize_t receive(uint8_t* buffer, size_t buffer_size);
    ssize_t send_to(const uint8_t* buffer, size_t buffer_size);

    const std::array<uint8_t, 6>& mac() const;
    int fd() const;
};
