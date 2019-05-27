#pragma once

#include "Name.hpp"

namespace Foundation
{
    class NameHolder
    {
    public:
        NameHolder(const char* string);

        operator Name() { return GetName(); }
        Name GetName();

    private:
        const char* m_String;
        Name m_Name;
    };
}
