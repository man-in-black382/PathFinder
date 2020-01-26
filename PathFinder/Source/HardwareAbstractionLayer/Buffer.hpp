#pragma once

#include "Resource.hpp"

#include <optional>

namespace HAL
{

    class Buffer : public Resource
    {
    public:
        Buffer(const Device& device, uint64_t size, ResourceState initialState, ResourceState expectedStates);
        Buffer(const Device& device, const Heap& heap, uint64_t heapOffset, uint64_t size, ResourceState initialState, ResourceState expectedStates);
        Buffer(const Device& device, const Heap& heap, uint64_t heapOffset, uint64_t size);
        Buffer(const Device& device, uint64_t size, CPUAccessibleHeapType heapType);

        ~Buffer();

        uint8_t* Map();
        void Unmap();

        virtual uint32_t SubresourceCount() const override;

        static ResourceFormat ConstructResourceFormat(const Device* device, uint64_t bufferSize);

    private:
        uint8_t* mMappedMemory = nullptr;
        uint64_t mSize = 0;
        std::optional<CPUAccessibleHeapType> mCPUAccessibleHeapType = std::nullopt;

    public:
        inline const auto Size() const { return mSize; }
        inline auto HeapType() const { return mCPUAccessibleHeapType; }
    };

}

