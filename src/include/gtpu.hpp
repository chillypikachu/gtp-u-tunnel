#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

std::vector<uint8_t> encode_gtpu(uint32_t teid, const uint8_t* ip_packet, size_t ip_size);

bool decode_gtpu(const uint8_t* packet, size_t packet_size, uint32_t& teid, const uint8_t*& payload, size_t& payload_size);
