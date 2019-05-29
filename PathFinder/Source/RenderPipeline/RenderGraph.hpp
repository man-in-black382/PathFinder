#pragma once

#include <vector>
#include <unordered_map>

#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/Device.hpp"

#include "RenderPassScheduler.hpp"
#include "RenderPass.hpp"

namespace PathFinder
{

    class RenderGraph : public IRenderPassScheduler
    {
    public:
        RenderGraph();

        virtual void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::Color dataFormat, 
            HAL::ResourceFormat::TextureKind kind, 
            const Geometry::Dimensions& dimensions) override;

        virtual void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::TypelessColor dataFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions) override;

        virtual void WillRenderToDepthStencil(
            Foundation::Name resourceName,
            HAL::ResourceFormat::DepthStencil dataFormat,
            const Geometry::Dimensions& dimensions) override;

        void AddRenderPass(const RenderPass& pass);

        void Schedule();
        void Render();

    private:
        using ResourceName = Foundation::Name;
        using PassName = Foundation::Name;
        using ResourceMap = std::unordered_map<ResourceName, HAL::Resource>;
        using ResourceStateMap = std::unordered_map<PassName, std::unordered_map<ResourceName, HAL::ResourceState>>;
        using ResourceAllocationMap = std::unordered_map<ResourceName, std::function<void()>>;

        HAL::DisplayAdapter FetchDefaultDisplayAdapter() const;

        HAL::Device mDevice;
        PassName mCurrentlySchedulingPassName;

        std::vector<RenderPass> mRenderPasses;
        std::vector<ResourceName> mRequestedResourc

        ResourceMap mResources;
        ResourceStateMap mResourceStateMap;
        ResourceAllocationMap mResourceDelayedAllocations;
    };

}
