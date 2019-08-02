#include "RenderEngine.hpp"

#include "../HardwareAbstractionLayer/DisplayAdapterFetcher.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

namespace PathFinder
{

    RenderEngine::RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath, const Scene* scene)
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
            mGraphicsDevice{ &mDevice, windowHandle, mDefaultRenderSurface, &mResourceManager, &mPipelineStateManager, &mVertexStorage },
            mContext{ scene, &mGraphicsDevice },
            mFrameFence { mDevice },
            mRingBuffer{ mDevice, 100, 3, 256, HAL::ResourceState::GenericRead, HAL::ResourceState::GenericRead, HAL::CPUAccessibleHeapType::Upload }
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

        OutputDebugString("Queueing commands\n");

        mFrameFence.IncreaseExpectedValue();

        mRingBuffer.NewFrameStarted(mFrameFence.ExpectedValue());
        mGraphicsDevice.mRingCommandList.NewFrameStarted(mFrameFence.ExpectedValue());

        MoveToNextBackBuffer();

        HAL::ColorTextureResource* currentBackBuffer = mGraphicsDevice.SwapChain().BackBuffers()[mCurrentBackBufferIndex].get();
        mGraphicsDevice.TransitionResource({ HAL::ResourceState::Present, HAL::ResourceState::RenderTarget, currentBackBuffer });

        for (auto& passPtr : mRenderPasses)
        {
            mResourceManager.SetCurrentPassName(passPtr->Name());
            TransitionResourceStates();
            passPtr->Render(&mContext);
        }

        mGraphicsDevice.TransitionResource({ HAL::ResourceState::RenderTarget, HAL::ResourceState::Present, currentBackBuffer });
        
        mGraphicsDevice.SwapChain().Present();

        mGraphicsDevice.mRingCommandList.CurrentCommandList().Close();
        mGraphicsDevice.mCommandQueue.ExecuteCommandList(mGraphicsDevice.mRingCommandList.CurrentCommandList());

        mGraphicsDevice.mCommandQueue.SignalFence(mFrameFence);
        mFrameFence.StallCurrentThreadUntilCompletion(3);

        mGraphicsDevice.mRingCommandList.CurrentCommandAllocator().Reset();
        mGraphicsDevice.mRingCommandList.CurrentCommandList().Reset(mGraphicsDevice.mRingCommandList.CurrentCommandAllocator());

        mRingBuffer.FrameCompleted(mFrameFence.CompletedValue());
        mGraphicsDevice.mRingCommandList.FrameCompleted(mFrameFence.CompletedValue());
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
        for (ResourceManager::ResourceName resourceName : mResourceManager.GetScheduledResourceNamesForCurrentPass())
        {
            HAL::ResourceState currentState = *mResourceManager.GetResourceCurrentState(resourceName);
            HAL::ResourceState nextState = *mResourceManager.GetResourceStateForCurrentPass(resourceName);
            HAL::Resource* resource = mResourceManager.GetResource(resourceName);

            if (currentState != nextState)
            {
                mGraphicsDevice.TransitionResource({ currentState, nextState, resource });
                mResourceManager.SetCurrentStateForResource(resourceName, nextState);
            }
        }
    }

}
