#include "include/raw_socket.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

RawSocket::RawSocket(const char* name) : fd_(socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL))), mac_address_{} {
    if (fd_ == -1) {
        throw std::runtime_error(std::string("RAW socket failed: ") + std::strerror(errno));
    }

    const unsigned int index = if_nametoindex(name);

    if (index == 0) {
        const int saved_errno = errno;
        close(fd_);
        throw std::runtime_error(std::string("Interface lookup failed: ") + std::strerror(saved_errno));
    }

    sockaddr_ll address{};
    address.sll_family = AF_PACKET;
    address.sll_protocol = htons(ETH_P_ALL);
    address.sll_ifindex = index;

    if (bind(fd_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1) {
        const int saved_errno = errno;
        close(fd_);
        throw std::runtime_error(std::string("RAW bind failed: ") + std::strerror(saved_errno));
    }

    ifreq request{};
    std::strncpy(request.ifr_name, name, IFNAMSIZ - 1);

    if (ioctl(fd_, SIOCGIFHWADDR, &request) == -1) {
        const int saved_errno = errno;
        close(fd_);
        throw std::runtime_error(std::string("MAC lookup failed: ") + std::strerror(saved_errno));
    }

    std::memcpy(mac_address_.data(), request.ifr_hwaddr.sa_data, mac_address_.size());
}

RawSocket::~RawSocket() {
    close(fd_);
}

ssize_t RawSocket::receive(uint8_t* buffer, size_t buffer_size) {
    return recv(fd_, buffer, buffer_size, 0);
}

ssize_t RawSocket::send_to(const uint8_t* buffer, size_t buffer_size) {
    return send(fd_, buffer, buffer_size, 0);
}

const std::array<uint8_t, 6>& RawSocket::mac() const {
    return mac_address_;
}

int RawSocket::fd() const {
    return fd_;
}
