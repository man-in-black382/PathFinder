#include "UIGPUStorage.hpp"

#include "../Foundation/Assert.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    UIGPUStorage::UIGPUStorage(const HAL::Device* device, CopyDevice* copyDevice, ResourceDescriptorStorage* descriptorStorage, uint8_t simulataneousFrameCount)
        : mDevice{ device }, mCopyDevice{ copyDevice }, mDescriptorStorage{ descriptorStorage }, mFrameCount{ simulataneousFrameCount } {}

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

    void UIGPUStorage::UploadUI()
    {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        
        UploadVertices(*drawData);
        UploadIndices(*drawData);
        UploadFont(ImGui::GetIO());
    }

    void UIGPUStorage::UploadVertices(const ImDrawData& drawData)
    {
        if (!mVertexBuffer || mVertexBuffer->Capacity() < drawData.TotalVtxCount)
        {
            mVertexBuffer = std::make_unique<HAL::RingBufferResource<ImDrawVert>>(
                *mDevice, GetVertexBufferPerFrameCapacity(drawData), mFrameCount, 128, HAL::CPUAccessibleHeapType::Upload);

            mVertexBuffer->SetDebugName("ImGUI Vertex Buffer");
        }

        if (drawData.CmdLists)
        {
            mVertexBuffer->Write(0, drawData.CmdLists[mCurrentFrameIndex]->VtxBuffer.Data, drawData.CmdLists[mCurrentFrameIndex]->VtxBuffer.Size);
        }
    }

    void UIGPUStorage::UploadIndices(const ImDrawData& drawData)
    {
        if (!mIndexBuffer || mIndexBuffer->Capacity() < drawData.TotalIdxCount)
        {
            mIndexBuffer = std::make_unique<HAL::RingBufferResource<ImDrawIdx>>(
                *mDevice, GetIndexBufferPerFrameCapacity(drawData), mFrameCount, 1, HAL::CPUAccessibleHeapType::Upload);

            mIndexBuffer->SetDebugName("ImGUI Index Buffer");
        }

        if (drawData.CmdLists)
        {
            mIndexBuffer->Write(0, drawData.CmdLists[mCurrentFrameIndex]->IdxBuffer.Data, drawData.CmdLists[mCurrentFrameIndex]->IdxBuffer.Size);
        }
    }

    void UIGPUStorage::UploadFont(const ImGuiIO& io)
    {
        uint8_t* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        auto byteCount = height * (width * 4);

        if (!mFontUploadBuffer || mFontUploadBuffer->Capacity() < byteCount)
        {
            mFontUploadBuffer = std::make_shared<HAL::BufferResource<uint8_t>>(*mDevice, byteCount, 1, HAL::CPUAccessibleHeapType::Upload);
            mFontUploadBuffer->Write(0, pixels, byteCount);
        }

        if (!mFontTexture || mFontTexture->Dimensions().Width != width || mFontTexture->Dimensions().Height != height)
        {
            mFontTexture = std::make_shared<HAL::TextureResource>(
                *mDevice, HAL::ResourceFormat::Color::RGBA8_Usigned_Norm, HAL::ResourceFormat::TextureKind::Texture2D,
                Geometry::Dimensions( width, height ), HAL::ResourceFormat::ColorClearValue{ 0.0, 0.0, 0.0, 0.0 },
                HAL::ResourceState::Common, HAL::ResourceState::AnyShaderAccess);

            mFontSRVIndex = mDescriptorStorage->EmplaceSRDescriptorIfNeeded(mFontTexture.get()).IndexInHeapRange();
            mCopyDevice->QueueBufferToTextureCopy(mFontUploadBuffer, mFontTexture, HAL::ResourceFootprint{ *mFontTexture });
        }
    }

    uint32_t UIGPUStorage::GetVertexBufferPerFrameCapacity(const ImDrawData& drawData) const
    {
        int32_t capacity = 0;

        for (auto i = 0; i < drawData.CmdListsCount; ++i)
        {
            capacity = std::max(capacity, drawData.CmdLists[i]->VtxBuffer.Size);
        }

        return std::max(capacity, 1);
    }

    uint32_t UIGPUStorage::GetIndexBufferPerFrameCapacity(const ImDrawData& drawData) const
    {
        int32_t capacity = 0;

        for (auto i = 0; i < drawData.CmdListsCount; ++i)
        {
            capacity = std::max(capacity, drawData.CmdLists[i]->IdxBuffer.Size);
        }

        return std::max(capacity, 1);
    }

}
