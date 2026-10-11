#include "include/packet_handlers.hpp"
#include "include/gtpu.hpp"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <vector>

void handle_downlink(RawSocket& raw_socket, GtpuSocket& gtpu_socket, const TunnelTable& table, const uint8_t* buffer, size_t size) {
    if (size < sizeof(ether_header)) return;

    ether_header ethernet{};
    std::memcpy(&ethernet, buffer, sizeof(ethernet));

    if (ntohs(ethernet.ether_type) == ETH_P_ARP) {
        if (size < sizeof(ether_header) + sizeof(ether_arp)) return;

        ether_arp arp{};
        std::memcpy(&arp, buffer + sizeof(ether_header), sizeof(arp));

        if (ntohs(arp.ea_hdr.ar_hrd) != ARPHRD_ETHER) return; //только Ethernet как аппаратный тип сети
        if (ntohs(arp.ea_hdr.ar_pro) != ETH_P_IP) return; //ARP для IPv4
        if (arp.ea_hdr.ar_hln != ETH_ALEN || arp.ea_hdr.ar_pln != 4) return;
        if (ntohs(arp.ea_hdr.ar_op) != ARPOP_REQUEST) return;

        uint32_t target_ip{};
        std::memcpy(&target_ip, arp.arp_tpa, sizeof(target_ip));

        if (table.find_by_ip(target_ip) == nullptr) return;

        ether_header reply_ethernet{};
        std::memcpy(reply_ethernet.ether_dhost, arp.arp_sha, ETH_ALEN); //arp_sha - MAC отправителя
        std::memcpy(reply_ethernet.ether_shost, raw_socket.mac().data(), ETH_ALEN);
        reply_ethernet.ether_type = htons(ETH_P_ARP);

        ether_arp reply_arp{};
        reply_arp.ea_hdr = arp.ea_hdr;
        reply_arp.ea_hdr.ar_op = htons(ARPOP_REPLY);

        std::memcpy(reply_arp.arp_sha, raw_socket.mac().data(), ETH_ALEN);
        std::memcpy(reply_arp.arp_spa, arp.arp_tpa, 4); //arp_spa - IP отправителя
        std::memcpy(reply_arp.arp_tha, arp.arp_sha, ETH_ALEN);
        std::memcpy(reply_arp.arp_tpa, arp.arp_spa, 4);

        std::array<uint8_t, sizeof(ether_header) + sizeof(ether_arp)> reply{};
        std::memcpy(reply.data(), &reply_ethernet, sizeof(reply_ethernet));
        std::memcpy(reply.data() + sizeof(reply_ethernet), &reply_arp, sizeof(reply_arp));

        const ssize_t sent = raw_socket.send_to(reply.data(), reply.size());

        if (sent == -1) {
            std::cerr << "ARP send error: " << std::strerror(errno) << std::endl;
        }

        return;
    }

    if (ntohs(ethernet.ether_type) != ETH_P_IP) return;

    const size_t layer3_size = size - sizeof(ether_header);
    if (layer3_size < sizeof(iphdr)) return;

    iphdr ip_header{};
    std::memcpy(&ip_header, buffer + sizeof(ether_header), sizeof(ip_header));

    if (ip_header.version != 4) return;

    const size_t ihl_bytes = static_cast<size_t>(ip_header.ihl) * 4; //Поле содержит длину в единицах по 4 байта
    const size_t total_size = ntohs(ip_header.tot_len);

    if (ihl_bytes < sizeof(iphdr) || ihl_bytes > layer3_size) return;
    if (total_size < ihl_bytes || total_size > layer3_size) return;
    if (total_size > 65507 - 8) return; //IP-пакет вместе с 8 байтами GTP-U помещается в максимально допустимую полезную нагрузку

    const UeSession* session = table.find_by_ip(ip_header.daddr);
    if (session == nullptr) return;

    const std::vector<uint8_t> gtpu_packet = encode_gtpu(session->teid_dl, buffer + sizeof(ether_header), total_size);
    const ssize_t sent = gtpu_socket.send_to_gnb(gtpu_packet);

    if (sent == -1) {
        std::cerr << "GTP-U send error: " << std::strerror(errno) << std::endl;
    } else if (static_cast<size_t>(sent) == gtpu_packet.size()) {
        std::cout << "DL: TEID=" << session->teid_dl << ", IP bytes=" << total_size << std::endl;
    }
}

void handle_uplink(RawSocket& raw_socket, const TunnelTable& table, const uint8_t* packet, size_t packet_size, const sockaddr_in& sender) {
    if (sender.sin_addr.s_addr != inet_addr("192.168.9.2")) return; //пакеты только от ожидаемого IP gNB
    if (ntohs(sender.sin_port) != 2152) return;

    uint32_t teid{};
    const uint8_t* ip_packet = nullptr;
    size_t ip_size = 0;

    if (!decode_gtpu(packet, packet_size, teid, ip_packet, ip_size)) return;

    const UeSession* session = table.find_by_teid_ul(teid);
    if (session == nullptr) return;
    if (ip_size < sizeof(iphdr)) return;

    iphdr ip_header{};
    std::memcpy(&ip_header, ip_packet, sizeof(ip_header));

    if (ip_header.version != 4) return;

    const size_t ihl_bytes = static_cast<size_t>(ip_header.ihl) * 4;
    const size_t total_size = ntohs(ip_header.tot_len);

    if (ihl_bytes < sizeof(iphdr) || ihl_bytes > ip_size) return;
    if (total_size < ihl_bytes || total_size > ip_size) return;

    if (ip_header.saddr != session->ip) {
        std::cerr << "Drop uplink: IP does not match TEID=" << teid << std::endl;
        return;
    }

    ether_header ethernet{};
    std::memset(ethernet.ether_dhost, 0xff, ETH_ALEN);
    std::memcpy(ethernet.ether_shost, raw_socket.mac().data(), ETH_ALEN);
    ethernet.ether_type = htons(ETH_P_IP);

    std::vector<uint8_t> frame(sizeof(ether_header) + total_size);
    std::memcpy(frame.data(), &ethernet, sizeof(ethernet));
    std::memcpy(frame.data() + sizeof(ethernet), ip_packet, total_size);

    const ssize_t sent = raw_socket.send_to(frame.data(), frame.size());

    if (sent == -1) {
        std::cerr << "Uplink send error: " << std::strerror(errno) << std::endl;
    } else if (static_cast<size_t>(sent) == frame.size()) {
        std::cout << "UL: TEID=" << teid << ", IP bytes=" << total_size << std::endl;
    }
}
