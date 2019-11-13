#pragma once

#include "RenderSurfaceDescription.hpp"
#include "PipelineResourceStorage.hpp"
#include "PipelineStateManager.hpp"
#include "VertexStorage.hpp"
#include "AssetResourceStorage.hpp"
#include "VertexLayouts.hpp"
#include "VertexStorageLocation.hpp"
#include "DrawablePrimitive.hpp"

#include "../Foundation/Name.hpp"
#include "../Foundation/Color.hpp"
#include "../Geometry/Dimensions.hpp"
#include "../HardwareAbstractionLayer/Viewport.hpp"
#include "../HardwareAbstractionLayer/ShaderRegister.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/RingCommandList.hpp"
#include "../HardwareAbstractionLayer/RenderTarget.hpp"

namespace PathFinder
{

    class GraphicsDevice
    {
    public:
        GraphicsDevice(
            const HAL::Device& device,
            const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
            PipelineResourceStorage* resourceStorage, 
            PipelineStateManager* pipelineStateManager,
            VertexStorage* vertexStorage, 
            AssetResourceStorage* assetStorage,
            RenderSurfaceDescription defaultRenderSurface,
            uint8_t simultaneousFramesInFlight
        );

        void SetRenderTarget(Foundation::Name resourceName);
        void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName = std::nullopt);
        void SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName);
        void ClearBackBuffer(const Foundation::Color& color);
        void ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color);
        void ClearDepth(Foundation::Name resourceName, float depthValue);
        void ApplyPipelineState(Foundation::Name psoName);
        void SetViewport(const HAL::Viewport& viewport);
        void Draw(uint32_t vertexCount, uint32_t vertexStart);
        void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount);
        void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart);
        void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount);
        void Draw(const VertexStorageLocation& vertexStorageLocation);
        void Draw(const DrawablePrimitive& primitive);
        void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);

        void BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);
        
        template <class T>
        void BindExternalBuffer(const HAL::BufferResource<T>& resource, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType);

        void WaitFence(HAL::Fence& fence);
        void ExecuteCommands();
        void SignalFence(HAL::Fence& fence);
        void ResetCommandList();
        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        void SetCurrentRenderPass(const RenderPass* pass);

    private:
        void ReapplyCommonGraphicsResourceBindings();
        void ReapplyCommonComputeResourceBindings();
        void ApplyDefaultViewportIfNeeded();

        HAL::GraphicsCommandQueue mCommandQueue;
        HAL::GraphicsRingCommandList mRingCommandList;
        
        const HAL::CBSRUADescriptorHeap* mUniversalGPUDescriptorHeap;
        const HAL::GraphicsPipelineState* mAppliedGraphicState;
        const HAL::ComputePipelineState* mAppliedComputeState;
        const HAL::RayTracingPipelineState* mAppliedRayTracingState;

        PipelineResourceStorage* mResourceStorage;
        PipelineStateManager* mPipelineStateManager;
        VertexStorage* mVertexStorage;
        AssetResourceStorage* mAssetStorage;

        const RenderPass* mCurrentRenderPass = nullptr;
        RenderSurfaceDescription mDefaultRenderSurface;
        std::optional<HAL::Viewport> mCurrentPassViewport;

    public:
        inline HAL::GraphicsCommandList& CommandList() { return mRingCommandList.CurrentCommandList(); }
        inline HAL::GraphicsCommandQueue& CommandQueue() { return mCommandQueue; }
    };

}

#include "GraphicsDevice.inl"