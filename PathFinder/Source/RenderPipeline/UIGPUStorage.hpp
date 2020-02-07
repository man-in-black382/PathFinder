#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/Texture.hpp"
#include "../Geometry/Rect2D.hpp"

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

        UIGPUStorage(const HAL::Device* device, uint8_t simulataneousFrameCount);

        void BeginFrame(uint64_t newFrameNumber);
        void EndFrame(uint64_t completedFrameNumber);

        void UploadUI();
        //void ReadbackPassDebugBuffer(Foundation::Name passName, const HAL::Buffer<float>& buffer);

    private:
        void UploadVertices(const ImDrawData& drawData);
        void UploadFont(const ImGuiIO& io);
        void ConstructMVP(const ImDrawData& drawData);

        uint8_t mFrameCount = 0;
        uint32_t mLastFenceValue = 0;
        uint32_t mCurrentFrameIndex = 0;
        const HAL::Device* mDevice = nullptr;
        uint64_t mFontSRVIndex;
        uint64_t mIndexCount;
        glm::mat4 mMVP;

        /*  std::unique_ptr<HAL::RingBufferResource<ImDrawVert>> mVertexBuffer;
          std::unique_ptr<HAL::RingBufferResource<ImDrawIdx>> mIndexBuffer;
          std::shared_ptr<HAL::Buffer<uint8_t>> mFontUploadBuffer;*/
        std::shared_ptr<HAL::Texture> mFontTexture;
        std::unordered_map<Foundation::Name, std::vector<float>> mPerPassDebugData;
        std::vector<DrawCommand> mDrawCommands;

    public:
        inline auto FontSRVIndex() const { return mFontSRVIndex; }
  /*      inline const auto VertexBuffer() const { return mVertexBuffer.get(); }
        inline const auto IndexBuffer() const { return mIndexBuffer.get(); }*/
        inline const auto& DrawCommands() const { return mDrawCommands; }
        inline const auto& MVP() const { return mMVP; }
    };

}
