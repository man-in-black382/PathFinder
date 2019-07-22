#pragma once

#include <vector>
#include <unordered_map>
#include <functional>

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"

#include "RenderPass.hpp"
#include "MeshGPUStorage.hpp"
#include "ResourceManager.hpp"
#include "GraphicsDevice.hpp"

namespace PathFinder
{

    class RenderGraph
    {
    public:
        RenderGraph(HWND windowHandle);

        void AddRenderPass(std::unique_ptr<RenderPass>&& pass);

        void Schedule();
        void Render();

    private:
        HAL::DisplayAdapter FetchDefaultDisplayAdapter() const;
        void MoveToNextBackBuffer();

        RenderSurface mDefaultRenderSurface;

        HAL::Device mDevice;
        GraphicsDevice mGraphicsDevice;
        MeshGPUStorage mMeshGPUStorage;
        ResourceManager mResourceManager;

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;

        uint8_t mCurrentBackBufferIndex = 0;

    public:
        inline auto& MeshStorage() { return mMeshGPUStorage; }
    };

}
