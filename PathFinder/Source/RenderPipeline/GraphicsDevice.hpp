#pragma once

#include "AsyncComputeDevice.hpp"

#include "VertexStorageLocation.hpp"
#include "DrawablePrimitive.hpp"

#include "../HardwareAbstractionLayer/Viewport.hpp"
#include "../HardwareAbstractionLayer/RenderTarget.hpp"

namespace PathFinder
{

    using GraphicsDeviceBase = AsyncComputeDevice<HAL::GraphicsCommandList, HAL::GraphicsCommandQueue>;

    class GraphicsDevice : public GraphicsDeviceBase
    {
    public:
        GraphicsDevice(
            const HAL::Device& device,
            const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
            Memory::PoolCommandListAllocator* commandListAllocator,
            Memory::ResourceStateTracker* resourceStateTracker,
            PipelineResourceStorage* resourceStorage,
            PipelineStateManager* pipelineStateManager,
            const RenderSurfaceDescription& defaultRenderSurface
        );

        void ApplyPipelineState(Foundation::Name psoName) override;

        void SetRenderTarget(Foundation::Name resourceName, std::optional<Foundation::Name> depthStencilResourceName = std::nullopt);
        void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName = std::nullopt);
        void ClearBackBuffer(const Foundation::Color& color);
        void ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color);
        void ClearDepth(Foundation::Name resourceName, float depthValue);
        void SetViewport(const HAL::Viewport& viewport);
        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
        void Draw(const DrawablePrimitive& primitive);
        
        void ResetViewportToDefault();
        
        void BindExternalBuffer(const Memory::Buffer& resource, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        template <class T>
        void SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace);

        template <size_t RTCount>
        void SetRenderTargets(
            const std::array<Foundation::Name, RTCount>& rtResourceNames,
            std::optional<Foundation::Name> depthStencilResourceName = std::nullopt);

        void SetBackBuffer(Memory::Texture* backBuffer);

    private:
        void ApplyStateIfNeeded(const HAL::GraphicsPipelineState* state);
        void ApplyStateIfNeeded(const HAL::ComputePipelineState* state) override;
        void ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state, const HAL::RayDispatchInfo* dispatchInfo) override;

        void ApplyCommonGraphicsResourceBindingsIfNeeded();
        void ApplyDefaultViewportIfNeeded();

        const HAL::GraphicsPipelineState* mAppliedGraphicsState = nullptr;
        const HAL::RootSignature* mAppliedGraphicsRootSignature = nullptr;
        const HAL::Buffer* mBoundGlobalConstantBufferGraphics = nullptr;
        const HAL::Buffer* mBoundFrameConstantBufferGraphics = nullptr;
        const HAL::Buffer* mBoundPassDebugBufferGraphics = nullptr;
        HAL::GPUAddress mBoundFrameConstantBufferAddressGraphics = 0;
        
        bool mRebindingAfterSignatureChangeRequired = false;
        std::optional<HAL::Viewport> mCurrentPassViewport;
        Memory::Texture* mBackBuffer = nullptr;
    };

}

#include "GraphicsDevice.inl"