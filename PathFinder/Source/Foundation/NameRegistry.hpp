#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Foundation
{
    class NameRegistry
    {
    public:
        static const uint64_t INVALID_ID = UINT64_MAX;

        static NameRegistry& SharedInstance();

        NameRegistry();
        ~NameRegistry();

        uint64_t ToId(const std::string& string);
        const std::string& ToString(uint64_t id);

    private:
        std::unordered_map<std::string, uint64_t> m_NameToId;
        std::vector<std::string> m_IdToName;
    };
}
