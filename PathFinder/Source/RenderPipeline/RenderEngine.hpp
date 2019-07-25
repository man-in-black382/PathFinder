#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"

#include "RenderPass.hpp"
#include "MeshGPUStorage.hpp"
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

        RenderSurface mDefaultRenderSurface;
        std::filesystem::path mExecutablePath;

        HAL::Device mDevice;
        MeshGPUStorage mMeshGPUStorage;
        ResourceManager mResourceManager;
        ShaderManager mShaderManager;
        PipelineStateManager mPipelineStateManager;
        GraphicsDevice mGraphicsDevice;

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;

        uint8_t mCurrentBackBufferIndex = 0;

    public:
        inline auto& MeshStorage() { return mMeshGPUStorage; }
    };

}
