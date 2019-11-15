#pragma once

#include <cstdint>

#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"

namespace PathFinder 
{

    struct Material
    {
        const HAL::TextureResource* AlbedoMap = nullptr;
        const HAL::TextureResource* NormalMap = nullptr;
        const HAL::TextureResource* RoughnessMap = nullptr;
        const HAL::TextureResource* MetalnessMap = nullptr; 
        const HAL::TextureResource* AOMap = nullptr;
        const HAL::TextureResource* DisplacementMap = nullptr;
        const HAL::TextureResource* DistanceAtlasIndirectionMap = nullptr;
        const HAL::TextureResource* DistanceAtlas = nullptr;
        const HAL::BufferResource<uint32_t>* DistanceAtlasCounter = nullptr;

        uint32_t AlbedoMapSRVIndex = 0;
        uint32_t NormalMapSRVIndex = 0;
        uint32_t RoughnessMapSRVIndex = 0;
        uint32_t MetalnessMapSRVIndex = 0;
        uint32_t AOMapSRVIndex = 0;
        uint32_t DisplacementMapSRVIndex = 0;
        uint32_t DistanceAtlasIndirectionMapSRVIndex = 0;
        uint32_t DistanceAtlasSRVIndex = 0;
    };

}
