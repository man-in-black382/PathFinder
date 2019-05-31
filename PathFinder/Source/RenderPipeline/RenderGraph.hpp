#pragma once

#include <vector>
#include <unordered_map>
#include <functional>

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

        void AddRenderPass(std::unique_ptr<RenderPass>&& pass);

        void Schedule();
        void Render();

    private:
        using ResourceName = Foundation::Name;
        using PassName = Foundation::Name;
        using ResourceMap = std::unordered_map<ResourceName, std::unique_ptr<HAL::Resource>>;
        using ResourceStateChainMap = std::unordered_map<PassName, std::unordered_map<ResourceName, HAL::ResourceState>>;
        using ResourceAllocationMap = std::unordered_map<ResourceName, std::function<void()>>;
        using ResourceExpectedStateMap = std::unordered_map<ResourceName, HAL::ResourceState>;

        void RegisterStateForResource(Foundation::Name resourceName, HAL::ResourceState state);

        template <class ResourceT, class ...Args>
        void QueueResourceAllocationIfNeeded(Foundation::Name resourceName, Args&&... args);

        HAL::DisplayAdapter FetchDefaultDisplayAdapter() const;


        HAL::Device mDevice;
        PassName mCurrentlySchedulingPassName;

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;
        std::vector<ResourceName> mRequestedResource;

        ResourceMap mResources;
        ResourceStateChainMap mResourceStateChainMap;
        ResourceExpectedStateMap mResourceExpectedStateMap;
        ResourceAllocationMap mResourceDelayedAllocations;
    };

}
