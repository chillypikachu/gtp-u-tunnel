#include "include/gtpu_socket.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

GtpuSocket::GtpuSocket() : fd_(socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) {
    if (fd_ == -1) {
        throw std::runtime_error(std::string("UDP socket failed: ") + std::strerror(errno));
    }

    sockaddr_in local{}; 
    local.sin_family = AF_INET; //IPv4
    local.sin_port = htons(2152);

    inet_pton(AF_INET, "192.168.9.1", &local.sin_addr);

    if (bind(fd_, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) == -1) {
        const int saved_errno = errno;
        close(fd_);
        throw std::runtime_error(std::string("UDP bind failed: ") + std::strerror(saved_errno));
    }

    gnb_.sin_family = AF_INET;
    gnb_.sin_port = htons(2152);

    inet_pton(AF_INET, "192.168.9.2", &gnb_.sin_addr);
}

GtpuSocket::~GtpuSocket() {
    close(fd_);
}

ssize_t GtpuSocket::send_to_gnb(const std::vector<uint8_t>& packet) {
    ssize_t result;

    do {
        result = sendto(fd_, packet.data(), packet.size(), 0, reinterpret_cast<const sockaddr*>(&gnb_), sizeof(gnb_));
    } while (result == -1 && errno == EINTR); //EINTR — системный вызов прерван сигналом

    return result;
}

ssize_t GtpuSocket::receive(uint8_t* buffer, size_t buffer_size, sockaddr_in& sender) {
    socklen_t sender_size = sizeof(sender);

    return recvfrom(fd_, buffer, buffer_size, 0, reinterpret_cast<sockaddr*>(&sender), &sender_size);
}

int GtpuSocket::fd() const {
    return fd_;
}
