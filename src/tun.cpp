#include "include/ue_session.hpp"
#include "include/raw_socket.hpp"
#include "include/gtpu_socket.hpp"
#include "include/packet_handlers.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    try {
        TunnelTable table;
        RawSocket raw_socket("upf_cn_veth");
        GtpuSocket gtpu_socket;

        const int epoll_fd = epoll_create1(EPOLL_CLOEXEC);

        if (epoll_fd == -1) {
            throw std::runtime_error(std::string("epoll_create1 failed: ") + std::strerror(errno));
        }

        struct EpollGuard {
            int fd;
            ~EpollGuard() { close(fd); }
        } epoll_guard{epoll_fd};

        epoll_event raw_event{};
        raw_event.events = EPOLLIN;
        raw_event.data.fd = raw_socket.fd();

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, raw_socket.fd(), &raw_event) == -1) {
            throw std::runtime_error(std::string("epoll add RAW failed: ") + std::strerror(errno));
        }

        epoll_event gtpu_event{};
        gtpu_event.events = EPOLLIN;
        gtpu_event.data.fd = gtpu_socket.fd();

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gtpu_socket.fd(), &gtpu_event) == -1) {
            throw std::runtime_error(std::string("epoll add UDP failed: ") + std::strerror(errno));
        }

        std::array<uint8_t, 65536> raw_buffer{};
        std::array<uint8_t, 65536> gtpu_buffer{};
        epoll_event events[2]{};

        std::cout << "UPF started with epoll" << std::endl;

        while (true) {
            const int count = epoll_wait(epoll_fd, events, 2, -1);

            if (count == -1) {
                if (errno == EINTR) continue;
                throw std::runtime_error(std::string("epoll_wait failed: ") + std::strerror(errno));
            }

            for (int i = 0; i < count; ++i) {
                const int ready_fd = events[i].data.fd;

                if (events[i].events & EPOLLHUP) {
                    throw std::runtime_error("Socket hangup");
                }

                if (events[i].events & EPOLLERR) {
                    int socket_error = 0;
                    socklen_t error_size = sizeof(socket_error);

                    if (getsockopt(ready_fd, SOL_SOCKET, SO_ERROR, &socket_error, &error_size) == -1) {
                        throw std::runtime_error(std::string("getsockopt failed: ") + std::strerror(errno));
                    }

                    if (socket_error != 0) {
                        std::cerr << "Socket error: " << std::strerror(socket_error) << std::endl;
                    }
                }

                if (!(events[i].events & EPOLLIN)) continue;

                if (ready_fd == raw_socket.fd()) {
                    const ssize_t received = raw_socket.receive(raw_buffer.data(), raw_buffer.size());

                    if (received == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
                        throw std::runtime_error(std::string("RAW receive failed: ") + std::strerror(errno));
                    }

                    if (received == 0) continue;

                    handle_downlink(raw_socket, gtpu_socket, table, raw_buffer.data(), static_cast<size_t>(received));
                }

                if (ready_fd == gtpu_socket.fd()) {
                    sockaddr_in sender{};
                    const ssize_t received = gtpu_socket.receive(gtpu_buffer.data(), gtpu_buffer.size(), sender);

                    if (received == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
                        std::cerr << "UDP receive error: " << std::strerror(errno) << std::endl;
                        continue;
                    }

                    if (received == 0) continue;

                    handle_uplink(raw_socket, table, gtpu_buffer.data(), static_cast<size_t>(received), sender);
                }
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }

    return 0;
}
