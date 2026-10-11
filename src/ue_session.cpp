#include "include/ue_session.hpp"

#include <arpa/inet.h>
#include <stdexcept>

UeSession make_ue_session(const char* str, uint32_t teid_dl, uint32_t teid_ul) {
    UeSession session{};

    if (inet_pton(AF_INET, str, &session.ip) != 1) {
        throw std::runtime_error("Invalid IP address");
    }

    session.teid_dl = teid_dl;
    session.teid_ul = teid_ul;

    return session;
}

TunnelTable::TunnelTable() {
    sessions_.push_back(make_ue_session("192.168.7.1", 2, 3));
    sessions_.push_back(make_ue_session("192.168.7.2", 4, 5));
    sessions_.push_back(make_ue_session("192.168.7.3", 6, 7));
    sessions_.push_back(make_ue_session("192.168.7.4", 8, 9));
}

const UeSession* TunnelTable::find_by_ip(uint32_t ip) const {
    for (const auto& session : sessions_) {
        if (session.ip == ip) return &session;
    }

    return nullptr;
}

const UeSession* TunnelTable::find_by_teid_ul(uint32_t teid_ul) const {
    for (const auto& session : sessions_) {
        if (session.teid_ul == teid_ul) return &session;
    }

    return nullptr;
}
