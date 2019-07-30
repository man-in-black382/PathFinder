#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"

namespace PathFinder
{

    RenderEngine::RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath)
        : mExecutablePath{ executablePath },
        mDefaultRenderSurface{
            { 1280, 720 },
            HAL::ResourceFormat::Color::RGBA8_Usigned_Norm,
            HAL::ResourceFormat::DepthStencil::Depth24_Float_Stencil8_Unsigned },

            mDevice{ FetchDefaultDisplayAdapter() },
            mVertexStorage{ &mDevice },
            mResourceManager{ &mDevice, mDefaultRenderSurface },
            mShaderManager{ mExecutablePath / "Shaders/" },
            mPipelineStateManager{ &mDevice, mDefaultRenderSurface },
            mGraphicsDevice{ &mDevice, windowHandle, mDefaultRenderSurface, &mResourceManager, &mPipelineStateManager }
    {
        mResourceManager.UseSwapChain(mGraphicsDevice.SwapChain());
    }

    void RenderEngine::AddRenderPass(std::unique_ptr<RenderPass>&& pass)
    {
        mRenderPasses.emplace_back(std::move(pass));
    }

    void RenderEngine::Schedule()
    {
        for (auto& passPtr : mRenderPasses)
        {
            mResourceManager.SetCurrentPassName(passPtr->Name());
            passPtr->SetupPipelineStates(&mShaderManager, &mPipelineStateManager);
            passPtr->ScheduleResources(&mResourceManager);
        }

        mResourceManager.AllocateScheduledResources();
        mPipelineStateManager.CompileStates();
    }

    void RenderEngine::Render()
    {
        if (mRenderPasses.empty()) return;

        MoveToNextBackBuffer();

        HAL::ColorTextureResource* currentBackBuffer = mGraphicsDevice.SwapChain().BackBuffers()[mCurrentBackBufferIndex].get();
        mGraphicsDevice.TransitionResource({ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer });

        for (auto& passPtr : mRenderPasses)
        {
            mResourceManager.SetCurrentPassName(passPtr->Name());
            TransitionResourceStates();
            passPtr->Render(&mGraphicsDevice);
        }

        mGraphicsDevice.TransitionResource({ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer });
        mGraphicsDevice.SwapChain().Present();
        mGraphicsDevice.ExecuteCommandBuffer();
    }

    HAL::DisplayAdapter RenderEngine::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch().back();
    }

    void RenderEngine::MoveToNextBackBuffer()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mGraphicsDevice.SwapChain().BackBuffers().size();
        mResourceManager.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
    }

    void RenderEngine::TransitionResourceStates()
    {
        const std::vector<ResourceManager::ResourceName>& resourceNames = mResourceManager.GetScheduledResourceNamesForCurrentPass();

        for (ResourceManager::ResourceName resourceName : resourceNames)
        {
            HAL::ResourceState currentState = *mResourceManager.GetResourceCurrentState(resourceName);
            HAL::ResourceState nextState = *mResourceManager.GetResourceStateForCurrentPass(resourceName);
            HAL::Resource* resource = mResourceManager.GetResource(resourceName);

            if (currentState != nextState)
            {
                mGraphicsDevice.TransitionResource({ nextState, nextState, resource });
                mResourceManager.SetCurrentStateForResource(resourceName, nextState);
            }
        }
    }

}
