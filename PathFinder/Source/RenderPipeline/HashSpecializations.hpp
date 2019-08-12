#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include <tuple>
#include <limits>

using NameNameTuple = std::tuple<Foundation::Name, Foundation::Name>;
using NameColorTuple = std::tuple<Foundation::Name, HAL::ResourceFormat::Color>;

namespace std
{

    template <>
    struct hash<NameNameTuple>
    {
        size_t operator()(const NameNameTuple& value) const
        {
            const auto name1Id = std::get<0>(value).ToId();
            const auto name2Id = std::get<1>(value).ToId();

            size_t hashValue = 0;
            hashValue |= name1Id;
            hashValue <<= std::numeric_limits<Foundation::Name::ID>::digits;
            hashValue |= name2Id;

            return hashValue;
        }
    };

    template <>
    struct hash<NameColorTuple>
    {
        size_t operator()(const NameColorTuple& value) const
        {
            const auto nameId = std::get<0>(value).ToId();
            const auto formatValue = std::underlying_type_t<HAL::ResourceFormat::Color>(std::get<1>(value));

            size_t hashValue = 0;
            hashValue |= nameId;
            hashValue <<= std::numeric_limits<Foundation::Name::ID>::digits;
            hashValue |= formatValue;

            return hashValue;
        }
    };

}
