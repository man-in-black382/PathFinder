#include "CommandRecorder.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    CommandRecorder::CommandRecorder(RenderDevice* graphicsDevice, const RenderPassGraph::Node* passNode)
        : mGraphicsDevice{ graphicsDevice }, mPassNode{ passNode } {}

    void CommandRecorder::SetRenderTarget(Foundation::Name rtName, std::optional<Foundation::Name> dsName)
    {
        mGraphicsDevice->SetRenderTarget(mPassNode, rtName, dsName);
    }

    void CommandRecorder::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> dsName)
    {
        mGraphicsDevice->SetBackBufferAsRenderTarget(mPassNode, dsName);
    }

    void CommandRecorder::ClearRenderTarget(Foundation::Name rtName)
    {
        mGraphicsDevice->ClearRenderTarget(mPassNode, rtName);
    }

    void CommandRecorder::ClearDepth(Foundation::Name dsName)
    {
        mGraphicsDevice->ClearDepth(mPassNode, dsName);
    }

    void CommandRecorder::ApplyPipelineState(Foundation::Name psoName)
    {
        mGraphicsDevice->ApplyPipelineState(mPassNode, psoName);
    }

    void CommandRecorder::SetViewport(const HAL::Viewport& viewport)
    {
        mGraphicsDevice->SetViewport(mPassNode, viewport);
    }

    void CommandRecorder::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        mGraphicsDevice->Draw(mPassNode, vertexCount, instanceCount);
    }
    
    void CommandRecorder::Draw(const DrawablePrimitive& primitive)
    {
        mGraphicsDevice->Draw(mPassNode, primitive);
    }

    void CommandRecorder::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        mGraphicsDevice->Dispatch(mPassNode, groupCountX, groupCountY, groupCountZ);
    }

    void CommandRecorder::Dispatch(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize)
    {
        float x = std::max(ceilf((float)viewportDimensions.Width / groupSize.Width), 1.f);
        float y = std::max(ceilf((float)viewportDimensions.Height / groupSize.Height), 1.f);
        float z = std::max(ceilf((float)viewportDimensions.Depth / groupSize.Depth), 1.f);
        
        Dispatch(x, y, z);
    }

    void CommandRecorder::DispatchRays(const Geometry::Dimensions& dispatchDimensions)
    {
        mGraphicsDevice->DispatchRays(mPassNode, dispatchDimensions.Width, dispatchDimensions.Height, dispatchDimensions.Depth);
    }

    void CommandRecorder::BindBuffer(Foundation::Name bufferName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindBuffer(mPassNode, bufferName, shaderRegister, registerSpace, registerType);
    }

    void CommandRecorder::BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindExternalBuffer(mPassNode, buffer, shaderRegister, registerSpace, registerType);
    }

}
