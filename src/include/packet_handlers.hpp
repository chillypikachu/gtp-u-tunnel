#pragma once

#include "raw_socket.hpp"
#include "gtpu_socket.hpp"
#include "ue_session.hpp"

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>

void handle_downlink(RawSocket& raw_socket, GtpuSocket& gtpu_socket, const TunnelTable& table, const uint8_t* buffer, size_t size);

void handle_uplink(RawSocket& raw_socket, const TunnelTable& table, const uint8_t* packet, size_t packet_size, const sockaddr_in& sender);
