#include "UIGPUStorage.hpp"

#include "../Foundation/Assert.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    UIGPUStorage::UIGPUStorage(const HAL::Device* device, CopyDevice* copyDevice, ResourceDescriptorStorage* descriptorStorage, uint8_t simulataneousFrameCount)
        : mDevice{ device }, mCopyDevice{ copyDevice }, mDescriptorStorage{ descriptorStorage }, mFrameCount{ simulataneousFrameCount } {}

    void UIGPUStorage::BeginFrame(uint64_t newFrameNumber)
    {
        if (mVertexBuffer) mVertexBuffer->PrepareMemoryForNewFrame(newFrameNumber);
        if (mIndexBuffer) mIndexBuffer->PrepareMemoryForNewFrame(newFrameNumber);

        mCurrentFrameIndex = (newFrameNumber - mLastFenceValue) % mFrameCount;

        ImGui::NewFrame();
    }

    void UIGPUStorage::EndFrame(uint64_t completedFrameNumber)
    {
        if (mVertexBuffer) mVertexBuffer->DiscardMemoryForCompletedFrames(completedFrameNumber);
        if (mIndexBuffer) mIndexBuffer->DiscardMemoryForCompletedFrames(completedFrameNumber);

        mLastFenceValue = completedFrameNumber;
    }

    void UIGPUStorage::UploadUI()
    {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        UploadVertices(*drawData);
        UploadFont(ImGui::GetIO());
        ConstructMVP(*drawData);

        //// Render command lists
        //int vtx_offset = 0;
        //int idx_offset = 0;
        //ImVec2 clip_off = draw_data->DisplayPos;
        //for (int n = 0; n < draw_data->CmdListsCount; n++)
        //{
        //    const ImDrawList* cmd_list = draw_data->CmdLists[n];
        //    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        //    {
        //        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
        //        if (pcmd->UserCallback)
        //        {
        //            // User callback (registered via ImDrawList::AddCallback)
        //            pcmd->UserCallback(cmd_list, pcmd);
        //        }
        //        else
        //        {
        //            // Apply Scissor, Bind texture, Draw
        //            const D3D12_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
        //            ctx->SetGraphicsRootDescriptorTable(1, *(D3D12_GPU_DESCRIPTOR_HANDLE*)& pcmd->TextureId);
        //            ctx->RSSetScissorRects(1, &r);
        //            ctx->DrawIndexedInstanced(pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
        //        }
        //        idx_offset += pcmd->ElemCount;
        //    }
        //    vtx_offset += cmd_list->VtxBuffer.Size;
        //}
    }

    void UIGPUStorage::ReadbackPassDebugBuffer(Foundation::Name passName, const HAL::BufferResource<float>& buffer)
    {
        buffer.Read([this, passName](const float* data)
        {
            auto amountOfWrittenFloats = uint32_t(data[0]);

            if (amountOfWrittenFloats == 0)
            {
                return;
            }

            auto& debugFloats = mPerPassDebugData[passName];
            debugFloats.assign(data + 1, data + amountOfWrittenFloats + 1);
        });
    }

    void UIGPUStorage::UploadVertices(const ImDrawData& drawData)
    {
        if (drawData.TotalVtxCount > 0 && (!mVertexBuffer || mVertexBuffer->PerFrameCapacity() < drawData.TotalVtxCount))
        {
            mVertexBuffer = std::make_unique<HAL::RingBufferResource<ImDrawVert>>(
                *mDevice, drawData.TotalVtxCount, mFrameCount, 1, HAL::CPUAccessibleHeapType::Upload);

            mVertexBuffer->SetDebugName("ImGUI Vertex Buffer");
        }

        if (drawData.TotalIdxCount > 0 && (!mIndexBuffer || mIndexBuffer->PerFrameCapacity() < drawData.TotalIdxCount))
        {
            mIndexBuffer = std::make_unique<HAL::RingBufferResource<ImDrawIdx>>(
                *mDevice, drawData.TotalIdxCount, mFrameCount, 1, HAL::CPUAccessibleHeapType::Upload);

            mIndexBuffer->SetDebugName("ImGUI Index Buffer");
        }

        mDrawCommands.clear();

        // All vertices and indices for all ImGui command lists and buffers
        // are stored in 1 vertex / 1 index buffer.
        // Track offsets as we fill in draw commands info.
        uint64_t vertexOffset = 0;
        uint64_t indexOffset = 0;
        glm::vec2 clipOffset{ drawData.DisplayPos.x, drawData.DisplayPos.y };

        for (auto cmdListIdx = 0; cmdListIdx < drawData.CmdListsCount; ++cmdListIdx)
        {
            const ImDrawList* imCommandList = drawData.CmdLists[cmdListIdx];

            mVertexBuffer->Write(vertexOffset, imCommandList->VtxBuffer.Data, imCommandList->VtxBuffer.Size);
            mIndexBuffer->Write(indexOffset, imCommandList->IdxBuffer.Data, imCommandList->IdxBuffer.Size);

            for (auto cmdBufferIdx = 0; cmdBufferIdx < imCommandList->CmdBuffer.Size; ++cmdBufferIdx)
            {
                DrawCommand& drawCommand = mDrawCommands.emplace_back();
                const ImDrawCmd* imCommand = &imCommandList->CmdBuffer[cmdBufferIdx];

                drawCommand.ScissorRect = Geometry::Rect2D{
                    {imCommand->ClipRect.x - clipOffset.x, imCommand->ClipRect.y - clipOffset.y},
                    {imCommand->ClipRect.z - clipOffset.x, imCommand->ClipRect.w - clipOffset.y}
                };

                drawCommand.VertexBufferOffset = vertexOffset;
                drawCommand.IndexBufferOffset = indexOffset;
                drawCommand.IndexCount = imCommand->ElemCount;

                indexOffset += imCommand->ElemCount;
            }

            vertexOffset += imCommandList->VtxBuffer.Size;
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

    void UIGPUStorage::ConstructMVP(const ImDrawData& drawData)
    {
        // Setup orthographic projection matrix
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
        float L = drawData.DisplayPos.x;
        float R = drawData.DisplayPos.x + drawData.DisplaySize.x;
        float T = drawData.DisplayPos.y;
        float B = drawData.DisplayPos.y + drawData.DisplaySize.y;

        mMVP =
        {
            { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
        };
    }

}
