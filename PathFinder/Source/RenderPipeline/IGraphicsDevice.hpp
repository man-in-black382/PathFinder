#pragma once

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"
#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/CommandAllocator.hpp"
#include "../HardwareAbstractionLayer/CommandList.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/Fence.hpp"
#include "../HardwareAbstractionLayer/RenderTarget.hpp"

#include "../Foundation/Name.hpp"
#include "../Foundation/Color.hpp"

#include "../Geometry/Dimensions.hpp"

#include "ResourceView.hpp"

namespace PathFinder
{

    class IGraphicsDevice
    {
    public:
        virtual void SetRenderTarget(const ResourceView<HAL::RTDescriptor>& view) = 0;
        virtual void SetRenderTargetAndDepthStencil(const ResourceView<HAL::RTDescriptor>& rtView, const ResourceView<HAL::DSDescriptor>& dsView) = 0;
        virtual void ClearRenderTarget(const Foundation::Color& color, const ResourceView<HAL::RTDescriptor>& rtView) = 0;
        virtual void ClearDepthStencil(float depthValue, const ResourceView<HAL::DSDescriptor>& dsView) = 0;

        //template <class Vertex> virtual void SetVertexBuffer(const HAL::BufferResource<Vertex>& vertexBuffer) = 0;
        //template <class Index> virtual void SetIndexBuffer(const HAL::BufferResource<Index>& indexBuffer) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t vertexStart) = 0;
        virtual void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount) = 0;
        virtual void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart) = 0;
        virtual void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount) = 0;
    };

}
