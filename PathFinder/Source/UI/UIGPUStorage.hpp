#pragma once

#include <Foundation/Name.hpp>
#include <Memory/GPUResourceProducer.hpp>
#include <Geometry/Rect2D.hpp>

#include <vector>
#include <memory>
#include <tuple>
#include <unordered_map>

#include <imgui/imgui.h>
#include <glm/mat4x4.hpp>

namespace PathFinder
{

    class UIGPUStorage
    {
    public:
        struct DrawCommand
        {
            uint64_t VertexBufferOffset;
            uint64_t IndexBufferOffset;
            uint64_t IndexCount;
            Geometry::Rect2D ScissorRect;
        };

        UIGPUStorage(Memory::GPUResourceProducer* resourceProducer);

        void StartNewFrame();
        void UploadUI();
        //void ReadbackPassDebugBuffer(Foundation::Name passName, const HAL::Buffer<float>& buffer);

    private:
        void UploadVertices(const ImDrawData& drawData);
        void UploadFont(const ImGuiIO& io);
        void ConstructMVP(const ImDrawData& drawData);

        Memory::GPUResourceProducer* mResourceProducer;
        uint64_t mIndexCount;
        glm::mat4 mMVP;

        Memory::GPUResourceProducer::BufferPtr mVertexBuffer;
        Memory::GPUResourceProducer::BufferPtr mIndexBuffer;
        Memory::GPUResourceProducer::TexturePtr mFontTexture;

        std::unordered_map<Foundation::Name, std::vector<float>> mPerPassDebugData;
        std::vector<DrawCommand> mDrawCommands;

    public:
        inline const auto VertexBuffer() const { return mVertexBuffer.get(); }
        inline const auto IndexBuffer() const { return mIndexBuffer.get(); }
        inline const auto FontTexture() const { return mFontTexture.get(); }
        inline const auto& DrawCommands() const { return mDrawCommands; }
        inline const auto& MVP() const { return mMVP; }
    };

}
