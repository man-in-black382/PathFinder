#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

namespace PathFinder
{

    RenderEngine::RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath, const Scene* scene)
        : mExecutablePath{ executablePath },
        mDefaultRenderSurface{ { 1280, 720 }, HAL::ResourceFormat::Color::RGBA8_Usigned_Norm, HAL::ResourceFormat::DepthStencil::Depth24_Float_Stencil8_Unsigned },
        mDevice{ FetchDefaultDisplayAdapter() },
        mFrameFence{ mDevice },
        mVertexStorage{ &mDevice },
        mResourceStorage{ &mDevice, mDefaultRenderSurface, mSimultaneousFramesInFlight },
        mResourceScheduler{ &mResourceStorage },
        mResourceProvider{ &mResourceStorage },
        mRootConstantsUpdater{ &mResourceStorage },
        mShaderManager{ mExecutablePath / "Shaders/" },
        mPipelineStateManager{ &mDevice, mDefaultRenderSurface },
        mGraphicsDevice{ mDevice, &mResourceStorage, &mPipelineStateManager, &mVertexStorage, mSimultaneousFramesInFlight },
        mContext{ scene, &mGraphicsDevice, &mRootConstantsUpdater },
        mSwapChain{ mGraphicsDevice.CommandQueue(), windowHandle, HAL::BackBufferingStrategy::Double, mDefaultRenderSurface.RenderTargetFormat(), mDefaultRenderSurface.Dimensions() }
    {
        mResourceStorage.UseSwapChain(mSwapChain);
    }

    void RenderEngine::AddRenderPass(std::unique_ptr<RenderPass>&& pass)
    {
        mRenderPasses.emplace_back(std::move(pass));
    }

    void RenderEngine::Schedule()
    {
        for (auto& passPtr : mRenderPasses)
        {
            mResourceStorage.SetCurrentPassName(passPtr->Name());
            passPtr->SetupPipelineStates(&mShaderManager, &mPipelineStateManager);
            passPtr->ScheduleResources(&mResourceScheduler);
        }

        mResourceStorage.AllocateScheduledResources();
        mPipelineStateManager.CompileStates();
    }

    void RenderEngine::Render()
    {
        if (mRenderPasses.empty()) return;

        //OutputDebugString("Queueing commands\n");

        mFrameFence.IncreaseExpectedValue();
        mResourceStorage.BeginFrame(mFrameFence.ExpectedValue());
        mGraphicsDevice.BeginFrame(mFrameFence.ExpectedValue());

        MoveToNextBackBuffer();

        HAL::ColorTextureResource* currentBackBuffer = mSwapChain.BackBuffers()[mCurrentBackBufferIndex].get();

        mGraphicsDevice.CommandList().TransitionResourceState(
            { HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer }
        );

        mGraphicsDevice.CommandList().SetGraphicsRootSignature(mPipelineStateManager.UniversalRootSignature());

        for (auto& passPtr : mRenderPasses)
        {
            mResourceStorage.SetCurrentPassName(passPtr->Name());
            TransitionResourceStates();
            SetRootConstantBuffer();
            passPtr->Render(&mContext);
        }

        mGraphicsDevice.CommandList().TransitionResourceState(
            { HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer }
        );
       
        mSwapChain.Present();
        mGraphicsDevice.ExecuteCommandsThenSignalFence(mFrameFence);
        mFrameFence.StallCurrentThreadUntilCompletion(mSimultaneousFramesInFlight);

        mResourceStorage.EndFrame(mFrameFence.CompletedValue());
        mGraphicsDevice.EndFrame(mFrameFence.CompletedValue());
    }

    HAL::DisplayAdapter RenderEngine::FetchDefaultDisplayAdapter() const
    {
        HAL::DisplayAdapterFetcher adapterFetcher;
        return adapterFetcher.Fetch().back();
    }

    void RenderEngine::MoveToNextBackBuffer()
    {
        mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mSwapChain.BackBuffers().size();
        mResourceStorage.SetCurrentBackBufferIndex(mCurrentBackBufferIndex);
    }

    void RenderEngine::SetRootConstantBuffer()
    {
        uint32_t index = 0;
        mGraphicsDevice.CommandList().SetGraphicsRootConstantBuffer(*mResourceStorage.GetRootConstantBufferForCurrentPass(), index);
    }

    void RenderEngine::TransitionResourceStates()
    {
        for (ResourceStorage::ResourceName resourceName : *mResourceStorage.GetScheduledResourceNamesForCurrentPass())
        {
            HAL::ResourceState currentState = *mResourceStorage.GetResourceCurrentState(resourceName);
            HAL::ResourceState nextState = *mResourceStorage.GetResourceStateForCurrentPass(resourceName);
            HAL::Resource* resource = mResourceStorage.GetResource(resourceName);

            if (currentState != nextState)
            {
                mGraphicsDevice.CommandList().TransitionResourceState({ currentState, nextState, resource });
                mResourceStorage.SetCurrentStateForResource(resourceName, nextState);
            }
        }
    }

}
