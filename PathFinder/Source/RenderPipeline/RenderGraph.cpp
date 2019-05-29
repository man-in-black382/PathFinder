#include "RenderGraph.hpp"

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
        HAL::ResourceState states;

        mResourceStateMap[mCurrentlySchedulingPassName][resourceName] = HAL::ResourceState::RenderTarget;

        if (mResources.find(resourceName) != mResources.end()) return;
            
        mResourceDelayedAllocations[resourceName] = [&]() {
            mResources.emplace(resourceName, HAL::ColorTextureResource{ mDevice, kind, dimensions, states, states });
        };
    }

    void RenderGraph::WillRenderToRenderTarget(
        Foundation::Name resourceName, HAL::ResourceFormat::TypelessColor dataFormat, 
        HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        HAL::ResourceState states;

        mResourceStateMap[mCurrentlySchedulingPassName][resourceName] = HAL::ResourceState::RenderTarget;

        if (mResources.find(resourceName) != mResources.end()) return;

        mResourceDelayedAllocations[resourceName] = [&]() {
            mResources.emplace(resourceName, HAL::TypelessTextureResource{ mDevice, dataFormat, kind, dimensions, states, states });
        };
    }

    void RenderGraph::WillRenderToDepthStencil(
        Foundation::Name resourceName, 
        HAL::ResourceFormat::DepthStencil dataFormat, 
        const Geometry::Dimensions& dimensions)
    {
        HAL::ResourceState states;

        mResourceStateMap[mCurrentlySchedulingPassName][resourceName] = HAL::ResourceState::DepthWrite;

        if (mResources.find(resourceName) != mResources.end()) return;

        mResourceDelayedAllocations[resourceName] = [&]() {
            mResources.emplace(resourceName, HAL::DepthStencilTextureResource{ mDevice, dataFormat, dimensions, states, states });
        };
    }

    void RenderGraph::AddRenderPass(const RenderPass& pass)
    {
        mRenderPasses.push_back(pass);
    }

    void RenderGraph::Schedule()
    {
        for (RenderPass& pass : mRenderPasses)
        {
            mCurrentlySchedulingPassName = pass.Name();
            pass.ScheduleResources(this);
        }
    }

    void RenderGraph::Render()
    {

    }

    HAL::DisplayAdapter RenderGraph::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch().back();
    }

}
