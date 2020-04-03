#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <optional>

namespace Foundation
{
    namespace Halton
    {
        uint32_t Prime(uint32_t n);

        template <uint32_t Dimensionality>
        std::array<float, Dimensionality> Element(uint32_t elementIndex, std::optional<std::array<uint32_t, Dimensionality>> customBases = std::nullopt);

        template <uint32_t Dimensionality>
        std::vector<std::array<float, Dimensionality>> Sequence(uint32_t elementStartIndex, uint32_t elementEndIndex);

        float Element(uint32_t elementIndex);
        std::vector<float> Sequence(uint32_t elementStartIndex, uint32_t elementEndIndex);
    }
}

#include "Halton.inl"