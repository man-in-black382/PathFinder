#pragma once

#include "GraphicCommandRecorder.hpp"
#include "RenderSurfaceDescription.hpp"
#include "PipelineResourceStorage.hpp"
#include "PipelineStateManager.hpp"
#include "VertexStorage.hpp"

#include <../HardwareAbstractionLayer/RingCommandList.hpp>

namespace PathFinder
{

    class GraphicsDevice : public GraphicCommandRecorder
    {
    public:
        GraphicsDevice(
            const HAL::Device& device,
            const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
            PipelineResourceStorage* resourceStorage, 
            PipelineStateManager* pipelineStateManager,
            VertexStorage* vertexStorage, 
            RenderSurfaceDescription defaultRenderSurface,
            uint8_t simultaneousFramesInFlight
        );

        virtual void SetRenderTarget(Foundation::Name resourceName) override;
        virtual void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName = std::nullopt) override;
        virtual void SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName) override;
        virtual void ClearBackBuffer(const Foundation::Color& color) override;
        virtual void ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color) override;
        virtual void ClearDepth(Foundation::Name resourceName, float depthValue) override;
        virtual void ApplyPipelineState(Foundation::Name psoName) override;
        virtual void UseVertexBufferOfLayout(VertexLayout layout) override;
        virtual void SetViewport(const HAL::Viewport& viewport) override;
        virtual void Draw(uint32_t vertexCount, uint32_t vertexStart) override;
        virtual void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount) override;
        virtual void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart) override;
        virtual void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount) override;
        virtual void Draw(const VertexStorageLocation& vertexStorageLocation) override;
        virtual void Draw(const DrawablePrimitive& primitive) override;
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

        void BeginFrame(uint64_t frameFenceValue);
        void ExecuteCommandsThenSignalFence(HAL::Fence& fence);
        void EndFrame(uint64_t completedFrameFenceValue);

        void SetCurrentRenderPass(const RenderPass* pass);

    private:
        void ReapplyCommonGraphicsResourceBindings();
        void ReapplyCommonComputeResourceBindings();
        void ApplyDefaultViewportIfNeeded();

        HAL::GraphicsCommandQueue mCommandQueue;
        HAL::GraphicsRingCommandList mRingCommandList;
        
        const HAL::CBSRUADescriptorHeap* mUniversalGPUDescriptorHeap;

        PipelineResourceStorage* mResourceStorage;
        PipelineStateManager* mPipelineStateManager;
        VertexStorage* mVertexStorage;

        const RenderPass* mCurrentRenderPass;
        RenderSurfaceDescription mDefaultRenderSurface;
        std::optional<HAL::Viewport> mCurrentPassViewport;

    public:
        inline HAL::GraphicsCommandList& CommandList() { return mRingCommandList.CurrentCommandList(); }
        inline HAL::GraphicsCommandQueue& CommandQueue() { return mCommandQueue; }
    };

}
