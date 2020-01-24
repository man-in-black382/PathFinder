#pragma once

#include <cstdint>

#include "../HardwareAbstractionLayer/Texture.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"

#include <glm/vec4.hpp>

namespace PathFinder 
{

    struct Material
    {
        const HAL::Texture* AlbedoMap = nullptr;
        const HAL::Texture* NormalMap = nullptr;
        const HAL::Texture* RoughnessMap = nullptr;
        const HAL::Texture* MetalnessMap = nullptr; 
        const HAL::Texture* AOMap = nullptr;
        const HAL::Texture* DisplacementMap = nullptr;
        const HAL::Texture* DistanceAtlasIndirectionMap = nullptr;
        const HAL::Texture* DistanceAtlas = nullptr;
        const HAL::Buffer* DistanceAtlasCounter = nullptr;

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
