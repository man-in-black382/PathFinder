#pragma once

#include <glm/vec4.hpp>
#include <cmath>

namespace PathFinder 
{

    struct CombinedLightingParams
    {
        enum class LightType : uint32_t
        {
            Disk = 0, Sphere = 1, Line = 2, Polygon = 3
        };

        float LuminousIntensity;
        float Width;
        float Height;
        float Radius;
        // 16 byte boundary

        glm::vec4 Orientation;
        // 16 byte boundary

        glm::vec4 Position;
        // 16 byte boundary

        std::underlying_type_t<LightType> LightTypeRaw;
    };

}
