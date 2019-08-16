#pragma once

namespace PathFinder
{

    template <class RootConstants>
    RootConstants* ResourceStorage::RootConstantDataForCurrentPass() const
    {
        auto bufferIt = mPerPassConstantBuffers.find(mCurrentPassName);
        if (bufferIt == mPerPassConstantBuffers.end()) return nullptr;

        return reinterpret_cast<RootConstants *>(bufferIt->second->At(0));
    }

    template <class BufferDataT>
    void ResourceStorage::AllocateRootConstantBufferIfNeeded()
    {
        auto bufferIt = mPerPassConstantBuffers.find(mCurrentPassName);
        bool alreadyAllocated = bufferIt != mPerPassConstantBuffers.end();

        if (alreadyAllocated) return;

        // Because we store complex objects in unified buffers of primitive type
        // we must alight manually beforehand and pass alignment of 1 to the buffer
        //
        auto bufferSize = Foundation::MemoryUtils::Align(sizeof(BufferDataT), 256);

        mPerPassConstantBuffers.emplace(mCurrentPassName, std::make_unique<HAL::RingBufferResource<uint8_t>>(
            *mDevice, bufferSize, mSimultaneousFramesInFlight, 1,
            HAL::ResourceState::GenericRead,
            HAL::ResourceState::GenericRead,
            HAL::CPUAccessibleHeapType::Upload)
        );
    }

}