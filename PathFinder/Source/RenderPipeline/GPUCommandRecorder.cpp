#include "GPUCommandRecorder.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    GPUCommandRecorder::GPUCommandRecorder(GraphicsDevice* graphicsDevice)
        : mGraphicsDevice{ graphicsDevice } {}

    void GPUCommandRecorder::SetRenderTarget(const ResourceKey& rtKey, std::optional<ResourceKey> dsKey)
    {
        mGraphicsDevice->SetRenderTarget(rtKey, dsKey);
    }

    void GPUCommandRecorder::SetBackBufferAsRenderTarget(std::optional<ResourceKey> dsKey)
    {
        mGraphicsDevice->SetBackBufferAsRenderTarget(dsKey);
    }

    void GPUCommandRecorder::ClearRenderTarget(const ResourceKey& rtKey)
    {
        mGraphicsDevice->ClearRenderTarget(rtKey);
    }

    void GPUCommandRecorder::ClearDepth(const ResourceKey& dsKey)
    {
        mGraphicsDevice->ClearDepth(dsKey);
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

    void GPUCommandRecorder::Dispatch(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize)
    {
        float x = std::max(ceilf((float)viewportDimensions.Width / groupSize.Width), 1.f);
        float y = std::max(ceilf((float)viewportDimensions.Height / groupSize.Height), 1.f);
        float z = std::max(ceilf((float)viewportDimensions.Depth / groupSize.Depth), 1.f);
        
        Dispatch(x, y, z);
    }

    void GPUCommandRecorder::DispatchRays(const Geometry::Dimensions& dispatchDimensions)
    {
        mGraphicsDevice->DispatchRays(dispatchDimensions.Width, dispatchDimensions.Height, dispatchDimensions.Depth);
    }

    void GPUCommandRecorder::BindBuffer(const ResourceKey& bufferKey, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindBuffer(bufferKey, shaderRegister, registerSpace, registerType);
    }

    void GPUCommandRecorder::BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindExternalBuffer(buffer, shaderRegister, registerSpace, registerType);
    }

}
