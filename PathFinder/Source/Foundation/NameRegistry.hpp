#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Foundation
{
    class NameRegistry
    {
    public:
        static const uint16_t INVALID_ID = UINT16_MAX;

        static NameRegistry& SharedInstance();

        NameRegistry();
        ~NameRegistry();

        uint16_t ToId(const std::string& string);
        const std::string& ToString(uint16_t id);

    private:
        std::unordered_map<std::string, uint16_t> m_NameToId;
        std::vector<std::string> m_IdToName;
    };
}
