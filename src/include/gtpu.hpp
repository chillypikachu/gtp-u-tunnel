#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <span>
#include <optional>

namespace gtpu {
inline constexpr std::size_t header_size = 8;
inline constexpr std::uint8_t flags = 0x30;
inline constexpr std::uint8_t g_pdu_message_type = 0xff;
}

std::vector<uint8_t> encode_gtpu(uint32_t teid, std::span<const uint8_t> ip_packet);

struct DecodedGtpu { uint32_t teid; std::span<const uint8_t> payload; };
std::optional<DecodedGtpu> decode_gtpu(std::span<const uint8_t> packet);
