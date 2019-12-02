#include "GPUCommandRecorder.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    GPUCommandRecorder::GPUCommandRecorder(GraphicsDevice* graphicsDevice)
        : mGraphicsDevice{ graphicsDevice } {}

    void GPUCommandRecorder::SetRenderTarget(Foundation::Name resourceName)
    {
        mGraphicsDevice->SetRenderTarget(resourceName);
    }

    void GPUCommandRecorder::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName)
    {
        mGraphicsDevice->SetBackBufferAsRenderTarget(depthStencilResourceName);
    }

    void GPUCommandRecorder::SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName)
    {
        mGraphicsDevice->SetRenderTargetAndDepthStencil(rtResourceName, dsResourceName);
    }

    void GPUCommandRecorder::ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color)
    {
        mGraphicsDevice->ClearRenderTarget(resourceName, color);
    }

    void GPUCommandRecorder::ClearBackBuffer(const Foundation::Color& color)
    {
        mGraphicsDevice->ClearBackBuffer(color);
    }

    void GPUCommandRecorder::ClearDepth(Foundation::Name resourceName, float depthValue)
    {
        mGraphicsDevice->ClearDepth(resourceName, depthValue);
    }

    void GPUCommandRecorder::ApplyPipelineState(Foundation::Name psoName)
    {
        mGraphicsDevice->ApplyPipelineState(psoName);
    }

    void GPUCommandRecorder::SetViewport(const HAL::Viewport& viewport)
    {
        mGraphicsDevice->SetViewport(viewport);
    }

    void GPUCommandRecorder::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        mGraphicsDevice->Draw(vertexCount, instanceCount);
    }
    
    void GPUCommandRecorder::Draw(const DrawablePrimitive& primitive)
    {
        mGraphicsDevice->Draw(primitive);
    }

    void GPUCommandRecorder::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        mGraphicsDevice->Dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void GPUCommandRecorder::BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindBuffer(resourceName, shaderRegister, registerSpace, registerType);
    }

}
