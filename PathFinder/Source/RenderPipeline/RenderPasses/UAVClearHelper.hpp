#pragma once

#include <cstdint>

#include <glm/vec4.hpp>

#include "../Geometry/Dimensions.hpp"
#include "../RenderContext.hpp"
#include "../RenderPassContentMediator.hpp"

namespace PathFinder
{

    struct UAVClearCBContent
    {
        enum class TextureType : uint32_t
        {
            Float = 0, UInt = 1
        };

        glm::vec4 FloatValues;
        glm::uvec4 UIntValues;
        TextureType Type;
        uint32_t OutputTexIdx;
    };

    void ClearUAVFloat(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::vec4& clearValues);
    void ClearUAVUInt(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::uvec4& clearValues);

}
