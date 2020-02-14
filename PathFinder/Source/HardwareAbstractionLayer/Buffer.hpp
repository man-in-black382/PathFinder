#pragma once

#include "Resource.hpp"

#include <optional>

namespace HAL
{

    class Buffer : public Resource
    {
    public:
        template <class Element = uint8_t>
        struct Properties
        {
            uint64_t ElementCapacity = 1;
            uint64_t ElementAlighnment = 1;
            ResourceState InitialState = ResourceState::Common;
            ResourceState ExpectedStates = ResourceState::Common;

            Properties(uint64_t capacity);
            Properties(uint64_t capacity, uint64_t alignment);
            Properties(uint64_t capacity, uint64_t alignment, ResourceState initialStates);
            Properties(uint64_t capacity, uint64_t alignment, ResourceState initialStates, ResourceState expectedStates);
        };

        template <class Element>
        Buffer(const Device& device, const Properties<Element>& properties, std::optional<CPUAccessibleHeapType> heapType = std::nullopt);

        template <class Element>
        Buffer(const Device& device, const Properties<Element>& properties, const Heap& heap, uint64_t heapOffset);

        ~Buffer();

        uint8_t* Map();
        void Unmap();

        virtual uint32_t SubresourceCount() const override;

        template <class Element = uint8_t>
        uint64_t ElementCapacity(uint64_t elementAlignment = 1) const;

        template <class Element>
        static ResourceFormat ConstructResourceFormat(const Device* device, const Properties<Element>& properties);

        bool CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const override;
        bool CanImplicitlyDecayToCommonStateFromState(ResourceState state) const override;

    private:
        template <class Element>
        static uint64_t WidthFromProperties(const Properties<Element>& properties);

        uint8_t* mMappedMemory = nullptr;
        uint64_t mRequestedSizeInBytes = 0;
        std::optional<CPUAccessibleHeapType> mCPUAccessibleHeapType = std::nullopt;

    public:
        inline auto HeapType() const { return mCPUAccessibleHeapType; }
    };

}

#include "Buffer.inl"