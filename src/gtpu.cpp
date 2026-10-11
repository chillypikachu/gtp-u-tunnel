#include "include/gtpu.hpp"

#include <arpa/inet.h>
#include <cstring>

std::vector<uint8_t> encode_gtpu(uint32_t teid, const uint8_t* ip_packet, size_t ip_size) {
    std::vector<uint8_t> packet(8 + ip_size);

    packet[0] = 0x30; 
    packet[1] = 0xff; //сообщ с пользоват данными

    const uint16_t length = htons(static_cast<uint16_t>(ip_size));
    const uint32_t network_teid = htonl(teid);

    std::memcpy(packet.data() + 2, &length, sizeof(length));
    std::memcpy(packet.data() + 4, &network_teid, sizeof(network_teid));
    std::memcpy(packet.data() + 8, ip_packet, ip_size);

    return packet;
}

bool decode_gtpu(const uint8_t* packet, size_t packet_size, uint32_t& teid, const uint8_t*& payload, size_t& payload_size) {
    if (packet_size < 8) return false;
    if (packet[0] != 0x30 || packet[1] != 0xff) return false;

    uint16_t network_length{};
    uint32_t network_teid{};

    std::memcpy(&network_length, packet + 2, sizeof(network_length));
    std::memcpy(&network_teid, packet + 4, sizeof(network_teid));

    payload_size = ntohs(network_length);

    if (payload_size != packet_size - 8) return false;

    teid = ntohl(network_teid);
    payload = packet + 8; //на внутр ip

    return true;
}
