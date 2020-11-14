#pragma once

#include <cstdint>

#include <glm/vec4.hpp>

#include <Geometry/Dimensions.hpp>

#include "../RenderContext.hpp"
#include "../RenderPassContentMediator.hpp"

namespace PathFinder
{

    struct UAVClearCBContent
    {
        enum class ClearOp : uint32_t
        {
            TextureFloat = 0, TextureUInt = 1, BufferFloat = 2, BufferUInt = 3
        };

        glm::vec4 FloatValues;
        glm::uvec4 UIntValues;
        ClearOp Operation;
        uint32_t OutputTexIdx;
        uint32_t BufferSize;
    };

    void ClearUAVTextureFloat(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::vec4& clearValues);
    void ClearUAVTextureUInt(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::uvec4& clearValues);
    void ClearUAVBufferFloat(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, float clearValue);
    void ClearUAVBufferUInt(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, uint32_t clearValue);

}
