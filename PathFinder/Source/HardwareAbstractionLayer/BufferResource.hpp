#pragma once

#include "Resource.hpp"

namespace HAL
{

    template <class T>
    class BufferResource : public Resource
    {
    public:
        using Resource::Resource;

        BufferResource(
            const Device& device,
            uint64_t capacity,
            uint64_t perElementAlignment, 
            ResourceState initialState,
            ResourceState expectedStates,
            std::optional<CPUAccessibleHeapType> heapType = std::nullopt
        );

        ~BufferResource();

        virtual void Write(uint64_t startIndex, const T* data, uint64_t dataLength = 1);
        virtual T* At(uint64_t index);

        virtual bool CanImplicitlyPromoteFromCommonStateToState(HAL::ResourceState state) const override;
        virtual bool CanImplicitlyDecayToCommonStateFromState(HAL::ResourceState state) const override;

    private:
        void ValidateMappedMemory() const;
        void ValidateIndex(uint64_t index) const;

    protected:
        uint64_t PaddedElementSize(uint64_t alignment);

        uint8_t* mMappedMemory = nullptr;
        uint64_t mNonPaddedElementSize = 0;
        uint64_t mPaddedElementSize = 0;
        uint64_t mCapacity = 0;

    public:
        inline const auto Capacity() const { return mCapacity; }
        inline const auto PaddedElementSize() const { return mPaddedElementSize; }
        inline const auto NonPaddedElementSize() const { return mNonPaddedElementSize; }
    };

}

#include "BufferResource.inl"

