#include "CommandRecorder.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    CommandRecorder::CommandRecorder(RenderDevice* graphicsDevice, const RenderPassGraph* passGraph, uint64_t graphNodeIndex)
        : mGraphicsDevice{ graphicsDevice }, mPassGraph{ passGraph }, mGraphNodeIndex{ graphNodeIndex } {}

    void CommandRecorder::SetRenderTarget(Foundation::Name rtName, std::optional<Foundation::Name> dsName)
    {
        mGraphicsDevice->SetRenderTarget(mPassGraph->Nodes()[mGraphNodeIndex], rtName, dsName);
    }

    void CommandRecorder::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> dsName)
    {
        mGraphicsDevice->SetBackBufferAsRenderTarget(mPassGraph->Nodes()[mGraphNodeIndex], dsName);
    }

    void CommandRecorder::ClearRenderTarget(Foundation::Name rtName)
    {
        mGraphicsDevice->ClearRenderTarget(mPassGraph->Nodes()[mGraphNodeIndex], rtName);
    }

    void CommandRecorder::ClearDepth(Foundation::Name dsName)
    {
        mGraphicsDevice->ClearDepth(mPassGraph->Nodes()[mGraphNodeIndex], dsName);
    }

    void CommandRecorder::ApplyPipelineState(Foundation::Name psoName)
    {
        mGraphicsDevice->ApplyPipelineState(mPassGraph->Nodes()[mGraphNodeIndex], psoName);
    }

    void CommandRecorder::SetViewport(const HAL::Viewport& viewport)
    {
        mGraphicsDevice->SetViewport(mPassGraph->Nodes()[mGraphNodeIndex], viewport);
    }

    void CommandRecorder::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        mGraphicsDevice->Draw(mPassGraph->Nodes()[mGraphNodeIndex], vertexCount, instanceCount);
    }
    
    void CommandRecorder::Draw(const DrawablePrimitive& primitive)
    {
        mGraphicsDevice->Draw(mPassGraph->Nodes()[mGraphNodeIndex], primitive);
    }

    void CommandRecorder::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        mGraphicsDevice->Dispatch(mPassGraph->Nodes()[mGraphNodeIndex], groupCountX, groupCountY, groupCountZ);
    }

    void CommandRecorder::Dispatch(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize)
    {
        auto dims = DispatchGroupCount(viewportDimensions, groupSize);
        Dispatch(dims.Width, dims.Height, dims.Depth);
    }

    void CommandRecorder::DispatchRays(const Geometry::Dimensions& dispatchDimensions)
    {
        mGraphicsDevice->DispatchRays(mPassGraph->Nodes()[mGraphNodeIndex], dispatchDimensions.Width, dispatchDimensions.Height, dispatchDimensions.Depth);
    }

    void CommandRecorder::BindBuffer(Foundation::Name bufferName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindBuffer(mPassGraph->Nodes()[mGraphNodeIndex], bufferName, shaderRegister, registerSpace, registerType);
    }

    void CommandRecorder::BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindExternalBuffer(mPassGraph->Nodes()[mGraphNodeIndex], buffer, shaderRegister, registerSpace, registerType);
    }

    Geometry::Dimensions CommandRecorder::DispatchGroupCount(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize)
    {
        uint64_t x = std::max(ceilf((float)viewportDimensions.Width / groupSize.Width), 1.f);
        uint64_t y = std::max(ceilf((float)viewportDimensions.Height / groupSize.Height), 1.f);
        uint64_t z = std::max(ceilf((float)viewportDimensions.Depth / groupSize.Depth), 1.f);

        return { x,y,z };
    }

}
