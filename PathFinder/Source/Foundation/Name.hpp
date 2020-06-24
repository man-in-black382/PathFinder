#pragma once

#include <string>
#include <cstdint>

namespace Foundation
{
    class Name
    {
    public:
        using ID = uint32_t;

        Name();
        Name(const std::string& string);
        Name(const char* cString);
        explicit Name(ID id);
        ~Name();

        Name(const Name& other);
        Name(Name&& other);

        Name& operator=(const Name& other);
        Name& operator=(Name&& other);

        bool operator==(const Name& other) const;
        bool operator<(const Name& other) const;

        const std::string& ToString() const;
        ID ToId() const;

        bool IsValid() const;

    private:
        ID m_Id;
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
