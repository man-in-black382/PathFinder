#pragma once

namespace PathFinder
{

    template <class RootConstants>
    RootConstants* ResourceStorage::GetRootConstantDataForCurrentPass() const
    {
        auto bufferIt = mPerpassConstantBuffers.find(mCurrentPassName);
        if (bufferIt == mPerpassConstantBuffers.end()) return nullptr;

        return reinterpret_cast<RootConstants *>(bufferIt->second->At(0));
    }

    template <class TextureT, class ...Args>
    void ResourceStorage::QueueTextureAllocationIfNeeded(ResourceName resourceName, const TextureAllocationCallback<TextureT>& callback, Args&&... args)
    {
        auto resourceIt = mResources.find(resourceName);

        bool isAlreadyAllocated = resourceIt != mResources.end();
        // Resource is already allocated, therefore scheduling stage is over,
        // why are you even here?
        if (isAlreadyAllocated) return;

        // Save callback until resource is created
        TexturePostAllocationActionMap<TextureT>& postAllocationActions = std::get<TexturePostAllocationActionMap<TextureT>>(mTexturePostAllocationActions);
        postAllocationActions[resourceName].push_back(callback);

        bool isWaitingForAllocation = mResourceAllocationActions.find(resourceName) != mResourceAllocationActions.end();
        if (isWaitingForAllocation) return;

        Foundation::Name passName = mCurrentPassName;

        mResourceAllocationActions[resourceName] = [this, passName, resourceName, args...]()
        {
            HAL::ResourceState initialState = mResourcePerPassStates[std::make_tuple(passName, resourceName)];
            HAL::ResourceState expectedStates = mResourceExpectedStates[resourceName];

            auto resource = std::make_unique<TextureT>(*mDevice, args..., initialState, expectedStates);

            // Call all callbacks associated with this resource name
            TexturePostAllocationActionMap<TextureT>& callbackMap = std::get<TexturePostAllocationActionMap<TextureT>>(mTexturePostAllocationActions);
            std::vector<TextureAllocationCallback<TextureT>>& callbackList = callbackMap[resourceName];

            for (TextureAllocationCallback<TextureT>& callback : callbackList)
            {
                callback(*resource);
            }

            // Store the actual resource
            mResources.emplace(resourceName, std::move(resource));
            mResourceCurrentStates[resourceName] = initialState;
        };
    }

    template <class BufferDataT>
    void ResourceStorage::AllocateRootConstantBufferIfNeeded()
    {
        auto bufferIt = mPerpassConstantBuffers.find(mCurrentPassName);
        bool alreadyAllocated = bufferIt != mPerpassConstantBuffers.end();

        if (alreadyAllocated) return;

        // Because we store complex objects in unified buffers of primitive type
        // we must alight manually beforehand and pass alignment of 1 to the buffer
        //
        auto bufferSize = Foundation::MemoryUtils::Align(sizeof(BufferDataT), 256);

        mPerpassConstantBuffers.emplace(mCurrentPassName, std::make_unique<HAL::RingBufferResource<uint8_t>>(
            *mDevice, bufferSize, mSimultaneousFramesInFlight, 1,
            HAL::ResourceState::GenericRead,
            HAL::ResourceState::GenericRead,
            HAL::CPUAccessibleHeapType::Upload)
        );
    }

}