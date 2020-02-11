#pragma once

#include "../Memory/GPUResourceProducer.hpp"

namespace PathFinder 
{

    struct Material
    {
        using TexturePtr = Memory::GPUResourceProducer::TexturePtr;

        TexturePtr AlbedoMap;
        TexturePtr NormalMap;
        TexturePtr RoughnessMap;
        TexturePtr MetalnessMap;
        TexturePtr AOMap;
        TexturePtr DisplacementMap;
        TexturePtr DistanceField;
    };

}
