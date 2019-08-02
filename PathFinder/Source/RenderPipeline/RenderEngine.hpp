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
#include "ResourceManager.hpp"
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
        void TransitionResourceStates();

        RenderSurface mDefaultRenderSurface;
        std::filesystem::path mExecutablePath;

        HAL::Device mDevice;
        VertexStorage mVertexStorage;
        ResourceManager mResourceManager;
        ShaderManager mShaderManager;
        PipelineStateManager mPipelineStateManager;
        GraphicsDevice mGraphicsDevice;
        RenderContext mContext;

        HAL::Fence mFrameFence;
        HAL::RingBufferResource<int> mRingBuffer;

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;

        uint8_t mCurrentBackBufferIndex = 0;

    public:
        inline VertexStorage& VertexGPUStorage() { return mVertexStorage; }
    };

}
