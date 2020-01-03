#pragma once

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"

#include "ResourceDescriptorStorage.hpp"
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
        UIGPUStorage(const HAL::Device* device, CopyDevice* copyDevice, ResourceDescriptorStorage* descriptorStorage, uint8_t simulataneousFrameCount);

        void BeginFrame(uint64_t newFrameNumber);
        void EndFrame(uint64_t completedFrameNumber);

        void UploadUI();

    private:
        void UploadVertices(const ImDrawData& drawData);
        void UploadIndices(const ImDrawData& drawData);
        void UploadFont(const ImGuiIO& io);
        uint32_t GetVertexBufferPerFrameCapacity(const ImDrawData& drawData) const;
        uint32_t GetIndexBufferPerFrameCapacity(const ImDrawData& drawData) const;

        uint8_t mFrameCount = 0;
        uint32_t mLastFenceValue = 0;
        uint32_t mCurrentFrameIndex = 0;
        const HAL::Device* mDevice = nullptr;
        CopyDevice* mCopyDevice;
        uint64_t mFontSRVIndex;
        ResourceDescriptorStorage* mDescriptorStorage;
        std::unique_ptr<HAL::RingBufferResource<ImDrawVert>> mVertexBuffer;
        std::unique_ptr<HAL::RingBufferResource<ImDrawIdx>> mIndexBuffer;
        std::shared_ptr<HAL::BufferResource<uint8_t>> mFontUploadBuffer;
        std::shared_ptr<HAL::TextureResource> mFontTexture;

    public:
        inline auto FontSRVIndex() const { return mFontSRVIndex; }
        inline const auto VertexBuffer() const { return mVertexBuffer.get(); }
        inline const auto IndexBuffer() const { return mIndexBuffer.get(); }
    };

}
