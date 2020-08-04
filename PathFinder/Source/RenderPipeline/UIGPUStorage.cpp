#include "UIGPUStorage.hpp"

#include "../Foundation/Assert.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    UIGPUStorage::UIGPUStorage(Memory::GPUResourceProducer* resourceProducer)
        : mResourceProducer{ resourceProducer } {}

    void UIGPUStorage::StartNewFrame()
    {
        ImGui::NewFrame();
    }

    void UIGPUStorage::UploadUI()
    {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        UploadVertices(*drawData);
        UploadFont(ImGui::GetIO());
        ConstructMVP(*drawData);
    }

    /*  void UIGPUStorage::ReadbackPassDebugBuffer(Foundation::Name passName, const HAL::Buffer<float>& buffer)
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
      }*/

    void UIGPUStorage::UploadVertices(const ImDrawData& drawData)
    {
        if ((!mVertexBuffer || mVertexBuffer->Capacity<ImDrawVert>() < drawData.TotalVtxCount) && drawData.TotalVtxCount > 0)
        {
            HAL::BufferProperties<ImDrawVert> properties{ (uint64_t)drawData.TotalVtxCount };
            mVertexBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
            mVertexBuffer->SetDebugName("ImGUI Vertex Buffer");
        }

        if ((!mIndexBuffer || mIndexBuffer->Capacity<ImDrawIdx>() < drawData.TotalIdxCount) && drawData.TotalIdxCount > 0)
        {
            HAL::BufferProperties<ImDrawIdx> properties{ (uint64_t)drawData.TotalIdxCount };
            mIndexBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
            mIndexBuffer->SetDebugName("ImGUI Index Buffer");
        }

        mDrawCommands.clear();

        // All vertices and indices for all ImGui command lists and buffers
        // are stored in 1 vertex / 1 index buffer.
        // Track offsets as we fill in draw commands info.
        uint64_t vertexOffset = 0;
        uint64_t indexOffset = 0;
        glm::vec2 clipOffset{ drawData.DisplayPos.x, drawData.DisplayPos.y };

        if (mVertexBuffer) mVertexBuffer->RequestWrite();
        if (mIndexBuffer) mIndexBuffer->RequestWrite();

        for (auto cmdListIdx = 0; cmdListIdx < drawData.CmdListsCount; ++cmdListIdx)
        {
            const ImDrawList* imCommandList = drawData.CmdLists[cmdListIdx];

            mVertexBuffer->Write(imCommandList->VtxBuffer.Data, vertexOffset, imCommandList->VtxBuffer.Size);
            mIndexBuffer->Write(imCommandList->IdxBuffer.Data, indexOffset, imCommandList->IdxBuffer.Size);

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

        if (!mFontTexture || mFontTexture->HALTexture()->Dimensions().Width != width || mFontTexture->HALTexture()->Dimensions().Height != height)
        {
            HAL::TextureProperties properties{
                HAL::ColorFormat::RGBA8_Usigned_Norm, HAL::TextureKind::Texture2D,
                Geometry::Dimensions{(uint64_t)width, (uint64_t)height}, HAL::ResourceState::AnyShaderAccess
            };

            mFontTexture = mResourceProducer->NewTexture(properties);
            mFontTexture->RequestWrite();
            mFontTexture->Write(pixels, 0, byteCount);
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
