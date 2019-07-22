#include "RenderGraph.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"

namespace PathFinder
{

    RenderGraph::RenderGraph(HWND windowHandle)
        : mDefaultRenderSurface{ 
            { 1280, 720 }, 
            HAL::ResourceFormat::Color::RGBA8_Usigned_Norm,
            HAL::ResourceFormat::DepthStencil::Depth24_Float_Stencil8_Unsigned },

            mDevice{ FetchDefaultDisplayAdapter() },
            mGraphicsDevice{ &mDevice, windowHandle, mDefaultRenderSurface },
            mMeshGPUStorage{ &mDevice },
            mResourceManager{ &mDevice, mDefaultRenderSurface }
    {
        mResourceManager.UseSwapChain(mGraphicsDevice.SwapChain());
    }

    void RenderGraph::AddRenderPass(std::unique_ptr<RenderPass>&& pass)
    {
        mRenderPasses.emplace_back(std::move(pass));
    }

    void RenderGraph::Schedule()
    {
        for (auto& passPtr : mRenderPasses)
        {
            mResourceManager.SetCurrentPassName(passPtr->Name());
            passPtr->ScheduleResources(&mResourceManager);
        }

        mResourceManager.AllocateScheduledResources();
    }

    void RenderGraph::Render()
    {
        if (mRenderPasses.empty()) return;

        MoveToNextBackBuffer();

        HAL::ColorTextureResource* currentBackBuffer = mGraphicsDevice.SwapChain().BackBuffers()[mCurrentBackBufferIndex].get();
        mGraphicsDevice.TransitionResource({ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer });

        for (auto& passPtr : mRenderPasses)
        {
            mResourceManager.SetCurrentPassName(passPtr->Name());

            std::vector<ResourceManager::ResourceName> names = mResourceManager.GetScheduledResourceNamesForPass(passPtr->Name());

            for (ResourceManager::ResourceName name : names)
            {
                HAL::ResourceState state = *mResourceManager.GetResourceStateForPass(passPtr->Name(), name);
                HAL::Resource* resource = mResourceManager.GetResource(name);

                mGraphicsDevice.TransitionResource({ state, state, resource });
            }

            passPtr->Render(&mResourceManager, &mGraphicsDevice);
        }

        mGraphicsDevice.TransitionResource({ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer });
        mGraphicsDevice.SwapChain().Present();
        mGraphicsDevice.FlushCommandBuffer();
    }

    HAL::DisplayAdapter RenderGraph::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch().back();
    }

    void RenderGraph::MoveToNextBackBuffer()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mGraphicsDevice.SwapChain().BackBuffers().size();
        mResourceManager.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
    }

}
