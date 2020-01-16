#pragma once

#include "AsyncComputeDevice.hpp"

#include "VertexStorageLocation.hpp"
#include "DrawablePrimitive.hpp"

#include "../HardwareAbstractionLayer/Viewport.hpp"
#include "../HardwareAbstractionLayer/RenderTarget.hpp"

namespace PathFinder
{

    using GraphicsDeviceBase = AsyncComputeDevice<HAL::GraphicsCommandList, HAL::GraphicsCommandAllocator, HAL::GraphicsCommandQueue>;

    class GraphicsDevice : public GraphicsDeviceBase
    {
    public:
        GraphicsDevice(
            const HAL::Device& device,
            const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
            PipelineResourceStorage* resourceStorage,
            PipelineStateManager* pipelineStateManager,
            const RenderSurfaceDescription& defaultRenderSurface,
            uint8_t simultaneousFramesInFlight
        );

        void ApplyPipelineState(Foundation::Name psoName) override;

        void SetRenderTarget(Foundation::Name resourceName);
        void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName = std::nullopt);
        void SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName);
        void ClearBackBuffer(const Foundation::Color& color);
        void ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color);
        void ClearDepth(Foundation::Name resourceName, float depthValue);
        void SetViewport(const HAL::Viewport& viewport);
        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1);
        void Draw(const DrawablePrimitive& primitive);
        
        void ResetViewportToDefault();
        
        template <class T>
        void BindExternalBuffer(const HAL::Buffer<T>& resource, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        template <class T>
        void SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace);

    private:
        void ApplyStateIfNeeded(const HAL::GraphicsPipelineState* state);
        void ApplyStateIfNeeded(const HAL::ComputePipelineState* state) override;
        void ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state) override;

        void ApplyCommonGraphicsResourceBindings();
        void BindCurrentPassBuffersGraphics();
        void ApplyDefaultViewportIfNeeded();

        const RenderPass* mCurrentRenderPass = nullptr;
        const HAL::GraphicsPipelineState* mAppliedGraphicsState;
        const HAL::RootSignature* mAppliedGraphicsRootSignature;
        
        std::optional<HAL::Viewport> mCurrentPassViewport;
    };

}

#include "GraphicsDevice.inl"