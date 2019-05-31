#include "RenderGraph.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"

namespace PathFinder
{

    RenderGraph::RenderGraph()
        : mDevice{ FetchDefaultDisplayAdapter() }
    {
        
    }

    void RenderGraph::WillRenderToRenderTarget(
        Foundation::Name resourceName, HAL::ResourceFormat::Color dataFormat,
        HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);
        QueueResourceAllocationIfNeeded<HAL::ColorTextureResource>(resourceName, mDevice, dataFormat, kind, dimensions);
    }

    void RenderGraph::WillRenderToRenderTarget(
        Foundation::Name resourceName, HAL::ResourceFormat::TypelessColor dataFormat, 
        HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);
        QueueResourceAllocationIfNeeded<HAL::TypelessTextureResource>(resourceName, mDevice, dataFormat, kind, dimensions);
    }

    void RenderGraph::WillRenderToDepthStencil(
        Foundation::Name resourceName, 
        HAL::ResourceFormat::DepthStencil dataFormat, 
        const Geometry::Dimensions& dimensions)
    {
        RegisterStateForResource(resourceName, HAL::ResourceState::DepthWrite);
        QueueResourceAllocationIfNeeded<HAL::DepthStencilTextureResource>(resourceName, mDevice, dataFormat, dimensions);
    }

    void RenderGraph::AddRenderPass(std::unique_ptr<RenderPass>&& pass)
    {
        mRenderPasses.emplace_back(std::move(pass));
    }

    void RenderGraph::Schedule()
    {
        for (auto& passPtr : mRenderPasses)
        {
            mCurrentlySchedulingPassName = passPtr->Name();
            passPtr->ScheduleResources(this);
        }

        for (auto& keyValue : mResourceDelayedAllocations)
        {
            auto& allocationAction = keyValue.second;
            allocationAction();
        }
    }

    void RenderGraph::Render()
    {
        for (auto& passPtr : mRenderPasses)
        {
            
        }
    }

    void RenderGraph::RegisterStateForResource(Foundation::Name resourceName, HAL::ResourceState state)
    {
        mResourceStateChainMap[mCurrentlySchedulingPassName][resourceName] = state;
        mResourceExpectedStateMap[resourceName] |= state;
    }

    template <class ResourceT, class ...Args>
    void RenderGraph::QueueResourceAllocationIfNeeded(Foundation::Name resourceName, Args&&... args)
    {
        bool isAlreadyAllocated = mResources.find(resourceName) != mResources.end();
        bool isWaitingForAllocation = mResourceDelayedAllocations.find(resourceName) != mResourceDelayedAllocations.end();

        if (isAlreadyAllocated || isWaitingForAllocation) return;

        Foundation::Name passName = mCurrentlySchedulingPassName;

        mResourceDelayedAllocations[resourceName] = [&, passName, resourceName]()
        {
            HAL::ResourceState initialState = mResourceStateChainMap[passName][resourceName];
            HAL::ResourceState expectedStates = mResourceExpectedStateMap[resourceName];

            mResources.emplace(resourceName, std::make_unique<ResourceT>(std::forward<Args>(args)..., initialState, expectedStates, HAL::HeapType::Default ));
        };
    }

    HAL::DisplayAdapter RenderGraph::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch().back();
    }

}
