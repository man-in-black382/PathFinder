#pragma once

#include <vector>

#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"

#include "RenderPassScheduler.hpp"
#include "RenderPass.hpp"

namespace PathFinder
{

    class RenderGraph : public IRenderPassScheduler
    {
    public:
        virtual void WillRenderToRenderTarget(HAL::ResourceFormat::Color dataFormat, HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions) override;
        virtual void WillRenderToRenderTarget(HAL::ResourceFormat::TypelessColor dataFormat, HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions) override;
        virtual void WillRenderToDepthStencil(HAL::ResourceFormat::DepthStencil dataFormat, const Geometry::Dimensions& dimensions) override;

        /*     using ResourceID = uint32_t;

        template <class ResourceT>
        ResourceT* GetExistingResource(ResourceState state);

        template <class BufferContentT>
        BufferResource<BufferContentT>* GetNewBufferResource(uint64_t capacity, HeapType heapType, ResourceState state);

        ColorTextureResource* GetNewColorTextureResource(
            ResourceFormat::Color dataType, ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions, ResourceState state
        );

        TypelessTextureResource* GetNewTypelessTextureResource(
            ResourceFormat::TypelessColor dataType, ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions, ResourceState state
        );

        DepthStencilTextureResource* GetNewDepthStencilTextureResource(
            ResourceFormat::DepthStencil dataType,
            const Geometry::Dimensions& dimensions,
            ResourceState state
        );*/

    private:

    };

}
