#pragma once

#include "../HardwareAbstractionLayer/RenderTarget.hpp"

#include "../Foundation/Name.hpp"
#include "../Foundation/Color.hpp"
#include "../Geometry/Dimensions.hpp"

#include "VertexLayouts.hpp"
#include "VertexStorageLocation.hpp"

namespace PathFinder
{

    class IGraphicsDevice
    {
    public:
        virtual void SetRenderTarget(Foundation::Name resourceName) = 0;
        virtual void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName) = 0;
        virtual void SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName) = 0;
        virtual void ClearBackBuffer(const Foundation::Color& color) = 0;
        virtual void ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color) = 0;
        virtual void ClearDepth(Foundation::Name resourceName, float depthValue) = 0;
        virtual void ApplyPipelineState(Foundation::Name psoName) = 0;
        virtual void UseVertexBufferOfLayout(VertexLayout layout) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t vertexStart) = 0;
        virtual void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount) = 0;
        virtual void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart) = 0;
        virtual void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount) = 0;
        virtual void Draw(const VertexStorageLocation& vertexStorageLocation) = 0;
    };

}
