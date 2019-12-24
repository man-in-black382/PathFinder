#include "UIGPUStorage.hpp"

#include "../Foundation/Assert.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    UIGPUStorage::UIGPUStorage(const HAL::Device* device, uint8_t simulataneousFramesCount)
        : mDevice{ device },  mFrameCount{ simulataneousFramesCount }
    {
       
    }

    void UIGPUStorage::BeginFrame(uint64_t frameFenceValue)
    {
        if (mVertexBuffer) mVertexBuffer->PrepareMemoryForNewFrame(frameFenceValue);
        if (mIndexBuffer) mIndexBuffer->PrepareMemoryForNewFrame(frameFenceValue);

        mCurrentFrameIndex = (frameFenceValue - mLastFenceValue) % mFrameCount;

        ImGui::NewFrame();
    }

    void UIGPUStorage::EndFrame(uint64_t completedFrameFenceValue)
    {
        if (mVertexBuffer) mVertexBuffer->DiscardMemoryForCompletedFrames(completedFrameFenceValue);
        if (mIndexBuffer) mIndexBuffer->DiscardMemoryForCompletedFrames(completedFrameFenceValue);

        mLastFenceValue = completedFrameFenceValue;
    }

    void UIGPUStorage::UploadUIVertices()
    {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        
        if (mVertexBuffer == nullptr || mVertexBuffer->Capacity() < drawData->TotalVtxCount)
        {
            mVertexBuffer = std::make_unique<HAL::RingBufferResource<ImDrawVert>>(
                *mDevice, GetVertexBufferPerFrameCapacity(drawData), mFrameCount, 128, HAL::CPUAccessibleHeapType::Upload);

            mVertexBuffer->SetDebugName("ImGUI Vertex Buffer");
        }

        if (mIndexBuffer == nullptr || mIndexBuffer->Capacity() < drawData->TotalIdxCount)
        {
            mIndexBuffer = std::make_unique<HAL::RingBufferResource<ImDrawIdx>>(
                *mDevice, GetIndexBufferPerFrameCapacity(drawData), mFrameCount, 1, HAL::CPUAccessibleHeapType::Upload);

            mIndexBuffer->SetDebugName("ImGUI Index Buffer");
        }

        mVertexBuffer->Write(0, drawData->CmdLists[mCurrentFrameIndex]->VtxBuffer.Data, drawData->CmdLists[mCurrentFrameIndex]->VtxBuffer.Size);
        mIndexBuffer->Write(0, drawData->CmdLists[mCurrentFrameIndex]->IdxBuffer.Data, drawData->CmdLists[mCurrentFrameIndex]->IdxBuffer.Size);
    }

    uint32_t UIGPUStorage::GetVertexBufferPerFrameCapacity(const ImDrawData* drawData) const
    {
        int32_t capacity = 0;

        for (auto i = 0; i < drawData->CmdListsCount; ++i)
        {
            capacity = std::max(capacity, drawData->CmdLists[i]->VtxBuffer.Size);
        }

        return capacity;
    }

    uint32_t UIGPUStorage::GetIndexBufferPerFrameCapacity(const ImDrawData* drawData) const
    {
        int32_t capacity = 0;

        for (auto i = 0; i < drawData->CmdListsCount; ++i)
        {
            capacity = std::max(capacity, drawData->CmdLists[i]->IdxBuffer.Size);
        }

        return capacity;
    }

}
