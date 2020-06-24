#include "NameRegistry.hpp"

namespace Foundation
{
    NameRegistry& NameRegistry::SharedInstance()
    {
        static NameRegistry registry;
        return registry;
    }

    NameRegistry::NameRegistry()
    {

    }

    NameRegistry::~NameRegistry()
    {

    }

    uint32_t NameRegistry::ToId(const std::string& string)
    {
        auto found = m_NameToId.find(string);

        if (found != m_NameToId.end())
        {
            return found->second;
        }

        m_IdToName.push_back(string);
        m_NameToId.insert({ string, m_IdToName.size() - 1 });

        return (uint32_t)m_IdToName.size() - 1;
    }

    const std::string& NameRegistry::ToString(uint32_t id)
    {
        return m_IdToName.at(id);
    }
}


