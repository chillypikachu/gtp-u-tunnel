#pragma once

#include <cstdint>
#include <vector>

struct UeSession {
    uint32_t ip;
    uint32_t teid_dl;
    uint32_t teid_ul;
};

UeSession make_ue_session(const char* str, uint32_t teid_dl, uint32_t teid_ul);

class TunnelTable {
    std::vector<UeSession> sessions_;

public:
    TunnelTable();

    const UeSession* find_by_ip(uint32_t ip) const;
    const UeSession* find_by_teid_ul(uint32_t teid_ul) const;
};
