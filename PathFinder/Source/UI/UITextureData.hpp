#pragma once

#include <Memory/Texture.hpp>

namespace PathFinder
{
   
    struct UITextureData
    {
        enum class SamplingMode : int32_t
        {
            Linear = 0, Point = 1
        };

        const Memory::Texture* Texture = nullptr;
        SamplingMode SamplerMode = SamplingMode::Point;
    };

}
