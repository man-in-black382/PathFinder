#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"

#include "../Scene/Scene.hpp"

#include "RenderPass.hpp"
#include "VertexStorage.hpp"
#include "ResourceStorage.hpp"
#include "ResourceScheduler.hpp"
#include "RootConstantsUpdater.hpp"
#include "GraphicsDevice.hpp"
#include "ShaderManager.hpp"
#include "PipelineStateManager.hpp"
#include "RenderContext.hpp"

namespace PathFinder
{

    class RenderEngine
    {
    public:
        RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath, const Scene* scene);

        void AddRenderPass(std::unique_ptr<RenderPass>&& pass);

        void Schedule();
        void Render();

    private:
        HAL::DisplayAdapter FetchDefaultDisplayAdapter() const; 
        void MoveToNextBackBuffer();
        void SetRootConstantBuffer();
        void TransitionResourceStates(PassName passName);

        uint8_t mCurrentBackBufferIndex = 0;
        uint8_t mSimultaneousFramesInFlight = 3;

        RenderSurface mDefaultRenderSurface;
        std::filesystem::path mExecutablePath;

        HAL::Device mDevice;
        HAL::Fence mFrameFence;

        VertexStorage mVertexStorage;
        ResourceStorage mResourceStorage;
        ResourceScheduler mResourceScheduler;
        ResourceProvider mResourceProvider;
        RootConstantsUpdater mRootConstantsUpdater;
        ShaderManager mShaderManager;
        PipelineStateManager mPipelineStateManager;
        GraphicsDevice mGraphicsDevice;
        RenderContext mContext;  

        HAL::SwapChain mSwapChain;

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;

    public:
        inline VertexStorage& VertexGPUStorage() { return mVertexStorage; }
    };

}
