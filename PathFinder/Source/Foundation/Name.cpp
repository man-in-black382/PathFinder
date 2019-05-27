#include "Name.hpp"
#include "NameRegistry.hpp"

#include <cassert>

namespace Foundation
{
    Name::Name()
        : m_Id{ NameRegistry::INVALID_ID }
    {

    }

    Name::Name(uint64_t id)
        : m_Id{ id }
    {

    }

    Name::Name(const std::string& string)
        : m_Id{ NameRegistry::INVALID_ID }
    {
        m_Id = NameRegistry::SharedInstance().ToId(string);
    }

    Name::~Name()
    {

    }

    Name::Name(const Name& other)
        : m_Id{ other.m_Id }
    {

    }

    Name::Name(Name&& other)
        : m_Id{ other.m_Id }
    {

    }

    Name& Name::operator=(const Name& other)
    {
        m_Id = other.m_Id;
        return *this;
    }

    Name& Name::operator=(Name&& other)
    {
        m_Id = other.m_Id;
        return *this;
    }

    uint64_t Name::ToId() const
    {
        assert(m_Id != NameRegistry::INVALID_ID);
        return m_Id;
    }

    bool Name::IsValid() const
    {
        return m_Id != NameRegistry::INVALID_ID;
    }

    const std::string& Name::ToSring() const
    {
        assert(m_Id != NameRegistry::INVALID_ID);
        return NameRegistry::SharedInstance().ToString(m_Id);
    }
}


