#pragma once

#include "Pool.hpp"
#include "Ring.hpp"

#include <HardwareAbstractionLayer/DescriptorHeap.hpp>
#include <HardwareAbstractionLayer/Buffer.hpp>
#include <HardwareAbstractionLayer/Texture.hpp>

#include <memory>
#include <functional>
#include <list>

namespace Memory
{
   
    class PoolDescriptorAllocator
    {
    public:
        PoolDescriptorAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight);

        template <class DescriptorT>
        using DescriptorPtr = std::unique_ptr<DescriptorT, std::function<void(DescriptorT*)>>;

        using RTDescriptorPtr = DescriptorPtr<HAL::RTDescriptor>;
        using DSDescriptorPtr = DescriptorPtr<HAL::DSDescriptor>;
        using SRDescriptorPtr = DescriptorPtr<HAL::SRDescriptor>;
        using UADescriptorPtr = DescriptorPtr<HAL::UADescriptor>;
        using CBDescriptorPtr = DescriptorPtr<HAL::CBDescriptor>;
        using SamplerDescriptorPtr = DescriptorPtr<HAL::SamplerDescriptor>;

        RTDescriptorPtr AllocateRTDescriptor(const HAL::Texture& texture, uint8_t mipLevel = 0, std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt);
        DSDescriptorPtr AllocateDSDescriptor(const HAL::Texture& texture);
        SRDescriptorPtr AllocateSRDescriptor(const HAL::Texture& texture, std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt);
        UADescriptorPtr AllocateUADescriptor(const HAL::Texture& texture, uint8_t mipLevel = 0, std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt);

        SRDescriptorPtr AllocateSRDescriptor(const HAL::Buffer& buffer, uint64_t stride);
        UADescriptorPtr AllocateUADescriptor(const HAL::Buffer& buffer, uint64_t stride);
        CBDescriptorPtr AllocateCBDescriptor(const HAL::Buffer& buffer, uint64_t stride);

        SamplerDescriptorPtr AllocateSamplerDescriptor(const HAL::Sampler& sampler);

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);
        
    private:
        template <class DescriptorT>
        struct Allocation
        {
            DescriptorT Descriptor;
            Pool<>::SlotType Slot;

            Allocation(const DescriptorT& descriptor, const Pool<>::SlotType& slot)
                : Descriptor{ descriptor }, Slot{ slot } {}
        };

        struct Deallocation
        {
            Pool<>::SlotType Slot;
            Pool<>* PoolPtr;

            Deallocation(const Pool<>::SlotType& slot, Pool<>* pool) 
                : Slot{ slot }, PoolPtr{ pool } {}
        };

        void ExecutePendingDeallocations(uint64_t frameIndex);
        void ValidateRTFormatsCompatibility(HAL::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat);
        void ValidateSRUAFormatsCompatibility(HAL::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat);

        uint64_t mDescriptorRangeCapacity = 1000;
        uint64_t mCurrentFrameIndex = 0;

        HAL::CBSRUADescriptorHeap mCBSRUADescriptorHeap;
        HAL::RTDescriptorHeap mRTDescriptorHeap;
        HAL::DSDescriptorHeap mDSDescriptorHeap;
        HAL::SamplerDescriptorHeap mSamplerDescriptorHeap;

        Ring mRingFrameTracker;

        Pool<> mRTPool;
        Pool<> mDSPool;
        Pool<> mSRPool;
        Pool<> mUAPool;
        Pool<> mCBPool;
        Pool<> mSamplerPool;

        std::list<Allocation<HAL::RTDescriptor>> mAllocatedRTDescriptors;
        std::list<Allocation<HAL::DSDescriptor>> mAllocatedDSDescriptors;
        std::list<Allocation<HAL::SRDescriptor>> mAllocatedSRDescriptors;
        std::list<Allocation<HAL::UADescriptor>> mAllocatedUADescriptors;
        std::list<Allocation<HAL::CBDescriptor>> mAllocatedCBDescriptors;
        std::list<Allocation<HAL::SamplerDescriptor>> mAllocatedSamplerDescriptors;

        std::vector<std::vector<Deallocation>> mPendingDeallocations;

    public:
        inline const HAL::CBSRUADescriptorHeap& CBSRUADescriptorHeap() const { return mCBSRUADescriptorHeap; }
        inline const HAL::SamplerDescriptorHeap& SamplerDescriptorHeap() const { return mSamplerDescriptorHeap; }
    };

}

