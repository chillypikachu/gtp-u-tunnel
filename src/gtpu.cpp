#include "include/gtpu.hpp"

#include <arpa/inet.h>
#include <cstring>

std::vector<uint8_t> encode_gtpu(uint32_t teid, std::span<const uint8_t> ip_packet) {
    std::vector<uint8_t> packet(gtpu::header_size + ip_packet.size());

    packet[0] = gtpu::flags;
    packet[1] = gtpu::g_pdu_message_type;

    const uint16_t length = htons(static_cast<uint16_t>(ip_packet.size()));
    const uint32_t network_teid = htonl(teid);

    std::memcpy(packet.data() + 2, &length, sizeof(length));
    std::memcpy(packet.data() + 4, &network_teid, sizeof(network_teid));
    std::memcpy(packet.data() + gtpu::header_size, ip_packet.data(), ip_packet.size());

    return packet;
}

std::optional<DecodedGtpu> decode_gtpu(std::span<const uint8_t> packet) {
    if (packet.size() < gtpu::header_size) return std::nullopt;
    if (packet[0] != gtpu::flags || packet[1] != gtpu::g_pdu_message_type) return std::nullopt;

    uint16_t network_length{};
    uint32_t network_teid{};

    std::memcpy(&network_length, packet.data() + 2, sizeof(network_length));
    std::memcpy(&network_teid, packet.data() + 4, sizeof(network_teid));

    const std::size_t payload_size = ntohs(network_length);

    if (payload_size != packet.size() - gtpu::header_size) return std::nullopt;
    return DecodedGtpu{ntohl(network_teid), packet.subspan(gtpu::header_size)};
}
