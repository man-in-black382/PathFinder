#pragma once

#include <string>
#include <cstdint>

namespace Foundation
{
    class Name
    {
    public:
        Name();
        Name(const class std::string& string);
        explicit Name(uint64_t id);
        ~Name();

        Name(const Name& other);
        Name(Name&& other);

        Name& operator=(const Name& other);
        Name& operator=(Name&& other);

        bool operator==(const Name& other) const;
        bool operator<(const Name& other) const;

        const class std::string& ToSring() const;
        uint64_t ToId() const;

        bool IsValid() const;

    private:
        uint64_t m_Id;
    };
}

inline bool Foundation::Name::operator==(const Name& other) const
{
    return m_Id == other.m_Id;
}

inline bool Foundation::Name::operator<(const Name& other) const
{
    return m_Id < other.m_Id;
}

namespace std
{
    template<>
    struct hash<Foundation::Name>
    {
        size_t operator()(const Foundation::Name& key) const
        {
            return key.ToId();
        }
    };
}
