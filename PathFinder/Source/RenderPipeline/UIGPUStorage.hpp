#pragma once

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"
#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"

#include "CopyDevice.hpp"

#include <vector>
#include <memory>
#include <tuple>

#include <imgui/imgui.h>

namespace PathFinder
{

    class UIGPUStorage
    {
    public:
        UIGPUStorage(const HAL::Device* device, uint8_t simulataneousFrameCount);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        void UploadUIVertices();

    private:
        void AllocateVertexBufferIfNeeded(const ImDrawData& drawData);
        void AllocateIndexBufferIfNeeded(const ImDrawData& drawData);
        void AllocateFontTextureIfNeeded(const ImGuiIO& io);
        uint32_t GetVertexBufferPerFrameCapacity(const ImDrawData& drawData) const;
        uint32_t GetIndexBufferPerFrameCapacity(const ImDrawData& drawData) const;

        uint8_t mFrameCount = 0;
        uint32_t mLastFenceValue = 0;
        uint32_t mCurrentFrameIndex = 0;
        const HAL::Device* mDevice = nullptr;
        std::unique_ptr<HAL::RingBufferResource<ImDrawVert>> mVertexBuffer;
        std::unique_ptr<HAL::RingBufferResource<ImDrawIdx>> mIndexBuffer;
        std::unique_ptr<HAL::TextureResource> mFontTexture;
    };

}
