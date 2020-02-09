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

        std::string AlbedoMapPath;
        std::string NormalMapPath;
        std::string RoughnessMapPath;
        std::string MetalnessMapPath;
        std::string AOMapPath;
        std::string DisplacmentMapPath;
        std::string DistanceFieldPath;

        bool DistanceFieldGenerated = true;
    };

}
