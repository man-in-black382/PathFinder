#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"

#include "RenderPass.hpp"
#include "VertexStorage.hpp"
#include "ResourceManager.hpp"
#include "GraphicsDevice.hpp"
#include "ShaderManager.hpp"
#include "PipelineStateManager.hpp"

namespace PathFinder
{

    class RenderEngine
    {
    public:
        RenderEngine(HWND windowHandle, const std::filesystem::path& executablePath);

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

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;

        uint8_t mCurrentBackBufferIndex = 0;

    public:
        inline VertexStorage& VertexGPUStorage() { return mVertexStorage; }
    };

}
