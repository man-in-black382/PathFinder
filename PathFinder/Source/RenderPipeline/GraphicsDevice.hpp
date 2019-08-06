#pragma once

#include "IGraphicsDevice.hpp"
#include "RenderSurface.hpp"
#include "ResourceStorage.hpp"
#include "PipelineStateManager.hpp"
#include "VertexStorage.hpp"

#include <../HardwareAbstractionLayer/RingCommandList.hpp>

namespace PathFinder
{

    class GraphicsDevice : public IGraphicsDevice
    {
    public:
        GraphicsDevice(
            const HAL::Device& device,
            const ResourceStorage* resourceManager, 
            const PipelineStateManager* pipelineStateManager,
            const VertexStorage* vertexStorage, 
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

        void BeginFrame(uint64_t frameFenceValue);
        void ExecuteCommandsThenSignalFence(HAL::Fence& fence);
        void EndFrame(uint64_t completedFrameFenceValue);

    private:
        HAL::GraphicsCommandQueue mCommandQueue;
        HAL::DirectRingCommandList mRingCommandList;

        const ResourceStorage* mResourceStorage;
        const PipelineStateManager* mPipelineStateManager;
        const VertexStorage* mVertexStorage;

    public:
        inline HAL::GraphicsCommandList& CommandList() { return mRingCommandList.CurrentCommandList(); }
        inline HAL::GraphicsCommandQueue& CommandQueue() { return mCommandQueue; }
    };

}
