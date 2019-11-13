#pragma once

#include "Resource.hpp"

#include <functional>

namespace HAL
{

    template <class T>
    class BufferResource : public Resource
    {
    public:
        using Resource::Resource;
        using ReadbackSession = std::function<void(const T*)>;

        BufferResource(const Device& device, uint64_t capacity, uint64_t perElementAlignment, ResourceState initialState, ResourceState expectedStates);
        BufferResource(const Device& device, const Heap& heap, uint64_t heapOffset, uint64_t capacity, uint64_t perElementAlignment, ResourceState initialState, ResourceState expectedStates);
        BufferResource(const Device& device, uint64_t capacity, uint64_t perElementAlignment, CPUAccessibleHeapType heapType);

        ~BufferResource();

        void Read(const ReadbackSession& session) const;

        virtual void Write(uint64_t startIndex, const T* data, uint64_t dataLength = 1);
        virtual T* At(uint64_t index);
        virtual uint32_t SubresourceCount() const override;

        static ResourceFormat ConstructResourceFormat(const Device* device, uint64_t capacity, uint64_t perElementAlignment);

    private:
        void ValidateMappedMemory() const;
        void ValidateIndex(uint64_t index) const;

    protected:
        uint64_t PaddedElementSize(uint64_t alignment);

        uint8_t* mMappedMemory = nullptr;
        uint64_t mNonPaddedElementSize = 0;
        uint64_t mPaddedElementSize = 0;
        uint64_t mCapacity = 0;
        uint64_t mPerElementAlignment = 1;

    public:
        inline const auto Capacity() const { return mCapacity; }
        inline const auto PaddedElementSize() const { return mPaddedElementSize; }
        inline const auto NonPaddedElementSize() const { return mNonPaddedElementSize; }
        inline const auto PerElementAlignment() const { return mPerElementAlignment; }
    };

}

#include "BufferResource.inl"

