#pragma once

#include <cstdint>

namespace HAL 
{

    enum class ShaderRegister : uint8_t
    {
        ShaderResource = 1 << 1,
        ConstantBuffer = 1 << 2, 
        UnorderedAccess = 1 << 3, 
        Sampler = 1 << 4
    };

}