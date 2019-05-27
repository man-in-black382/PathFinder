#include "NameHolder.hpp"

Foundation::NameHolder::NameHolder(const char* string)
    : m_String{ string }
{

}

Foundation::Name Foundation::NameHolder::GetName()
{
    if (!m_Name.IsValid())
    {
        std::string nameString{ m_String };
        m_Name = Name(nameString);
    }

    return m_Name;
}
