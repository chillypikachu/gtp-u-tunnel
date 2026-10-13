#pragma once

#include "raw_socket.hpp"
#include "gtpu_socket.hpp"
#include "ue_session.hpp"

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <span>

void handle_downlink(RawSocket& raw_socket, GtpuSocket& gtpu_socket, const TunnelTable& table, std::span<const uint8_t> buffer);

void handle_uplink(RawSocket& raw_socket, const TunnelTable& table, std::span<const uint8_t> packet, const sockaddr_in& sender);
