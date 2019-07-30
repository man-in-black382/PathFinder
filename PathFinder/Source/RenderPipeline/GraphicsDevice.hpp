#pragma once

#include "IGraphicsDevice.hpp"
#include "RenderSurface.hpp"
#include "ResourceManager.hpp"
#include "PipelineStateManager.hpp"

namespace PathFinder
{

    class GraphicsDevice : public IGraphicsDevice
    {
    public:
        GraphicsDevice(
            HAL::Device* device, HWND windowHandle, const RenderSurface& renderSurface,
            ResourceManager* resourceManager, PipelineStateManager* pipelineStateManager
        );

      /*  virtual void SetRenderTarget(const ResourceView<HAL::RTDescriptor>& view) override;
        virtual void SetRenderTargetAndDepthStencil(const ResourceView<HAL::RTDescriptor>& rtView, const ResourceView<HAL::DSDescriptor>& dsView) override;
        virtual void ClearRenderTarget(const Foundation::Color& color, const ResourceView<HAL::RTDescriptor>& rtView) override;
        virtual void ClearDepthStencil(float depthValue, const ResourceView<HAL::DSDescriptor>& dsView) override;*/

        //template <class Vertex> virtual void SetVertexBuffer(const HAL::BufferResource<Vertex>& vertexBuffer) override;
        //template <class Index> virtual void SetIndexBuffer(const HAL::BufferResource<Index>& indexBuffer) override;

        //virtual void ApplyPipelineState(const HAL::GraphicsPipelineState& state) override;

        virtual void SetRenderTarget(Foundation::Name resourceName) override;
        virtual void SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName = std::nullopt) override;
        virtual void SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName) override;
        virtual void ClearBackBuffer(const Foundation::Color& color) override;
        virtual void ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color) override;
        virtual void ClearDepthStencil(Foundation::Name resourceName, float depthValue) override;
        virtual void ApplyPipelineState(Foundation::Name psoName) override;

        virtual void Draw(uint32_t vertexCount, uint32_t vertexStart) override;
        virtual void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount) override;
        virtual void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart) override;
        virtual void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount) override;

        void TransitionResource(const HAL::ResourceTransitionBarrier& barrier);
        void ExecuteCommandBuffer();

    private:
        HAL::Device* mDevice;
        HAL::DirectCommandAllocator mCommandAllocator;
        HAL::DirectCommandList mCommandList;
        HAL::DirectCommandQueue mCommandQueue;
        HAL::Fence mFence;
        HAL::SwapChain mSwapChain;

        ResourceManager* mResourceManager;
        PipelineStateManager* mPipelineStateManager;

    public:
        inline HAL::SwapChain& SwapChain() { return mSwapChain; }
    };

  /*  template <class Index>
    void PathFinder::GraphicsDevice::SetIndexBuffer(const HAL::BufferResource<Index>& indexBuffer)
    {

    }

    template <class Vertex>
    void PathFinder::GraphicsDevice::SetVertexBuffer(const HAL::BufferResource<Vertex>& vertexBuffer)
    {

    }*/


}
