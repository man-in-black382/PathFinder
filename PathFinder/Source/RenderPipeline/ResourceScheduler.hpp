#pragma once

#include "ResourceStorage.hpp"

namespace PathFinder
{

    class ResourceScheduler
    {
    public:
        ResourceScheduler(ResourceStorage* manager);

        template <class BufferDataT>
        void WillUseRootConstantBuffer();

        void WillRenderToRenderTarget(Foundation::Name resourceName);

        void WillRenderToDepthStencil(Foundation::Name resourceName);

        void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::Color dataFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions);

        void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::TypelessColor dataFormat,
            HAL::ResourceFormat::Color shaderVisisbleFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions);

        void WillRenderToDepthStencil(
            Foundation::Name resourceName,
            HAL::ResourceFormat::DepthStencil dataFormat,
            const Geometry::Dimensions& dimensions);

    private:
        ResourceStorage* mResourceStorage;
    };

    template <class BufferDataT>
    void PathFinder::ResourceScheduler::WillUseRootConstantBuffer()
    {

    }

}
