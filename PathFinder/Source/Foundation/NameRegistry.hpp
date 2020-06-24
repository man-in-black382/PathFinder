#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Foundation
{
    class NameRegistry
    {
    public:
        static const uint32_t INVALID_ID = UINT32_MAX;

        static NameRegistry& SharedInstance();

        NameRegistry();
        ~NameRegistry();

        uint32_t ToId(const std::string& string);
        const std::string& ToString(uint32_t id);

    private:
        std::unordered_map<std::string, uint32_t> m_NameToId;
        std::vector<std::string> m_IdToName;
    };
}
