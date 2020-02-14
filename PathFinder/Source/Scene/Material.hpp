#pragma once

#include "../Memory/GPUResourceProducer.hpp"

namespace PathFinder 
{

    struct Material
    {
        Memory::Texture* AlbedoMap = nullptr;
        Memory::Texture* NormalMap = nullptr;
        Memory::Texture* RoughnessMap = nullptr;
        Memory::Texture* MetalnessMap = nullptr;
        Memory::Texture* AOMap = nullptr;
        Memory::Texture* DisplacementMap = nullptr;
        Memory::Texture* DistanceField = nullptr;
    };

}
