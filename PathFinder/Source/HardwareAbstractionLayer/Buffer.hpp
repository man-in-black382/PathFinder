#pragma once

#include "Resource.hpp"

#include <optional>

namespace HAL
{

    class Buffer : public Resource
    {
    public:
        template <class Element>
        Buffer(const Device& device, const BufferProperties<Element>& properties, std::optional<CPUAccessibleHeapType> heapType = std::nullopt);

        template <class Element>
        Buffer(const Device& device, const BufferProperties<Element>& properties, const Heap& heap, uint64_t heapOffset);

        ~Buffer();

        uint8_t* Map();
        void Unmap();

        virtual uint32_t SubresourceCount() const override;

        template <class Element = uint8_t>
        uint64_t ElementCapacity(uint64_t elementAlignment = 1) const;

        bool CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const override;
        bool CanImplicitlyDecayToCommonStateFromState(ResourceState state) const override;

    private:
        uint8_t* mMappedMemory = nullptr;
        BufferProperties<uint8_t> mProperties;
        std::optional<CPUAccessibleHeapType> mCPUAccessibleHeapType = std::nullopt;

    public:
        inline auto HeapType() const { return mCPUAccessibleHeapType; }
        inline auto RequestedMemory() const { return mProperties.ElementCapacity; }
        inline const auto& Properties() const { return mProperties; }
    };

}

#include "Buffer.inl"