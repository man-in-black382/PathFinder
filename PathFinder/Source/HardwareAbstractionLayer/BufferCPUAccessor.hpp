#pragma once

#include "Buffer.hpp"

#include <functional>

namespace HAL
{

    template <class T>
    class BufferCPUAccessor
    {
    public:
        using ReadbackSession = std::function<void(const T*)>;
        using DeallocationCallback = std::function<void()>;

        BufferCPUAccessor(Buffer* buffer, uint64_t elementCapacity, uint64_t elementPadding);
        ~BufferCPUAccessor();

        void Read(const ReadbackSession& session) const;
        void Write(uint64_t startIndex, const T* data, uint64_t dataLength = 1);
        T* GetWriteOnlyPointer(uint64_t index);

        void SetGlobalOffsetInObjects(uint64_t offset);
        void SetDeallocationCallback(const DeallocationCallback& callback);

    protected:
        Buffer* mBuffer = nullptr;
        uint64_t mNonPaddedElementSize = 0;
        uint64_t mPaddedElementSize = 0;
        uint64_t mCapacity = 0;
        uint64_t mPerElementAlignment = 1;
        uint64_t mGlobalObjectOffset = 0;
        DeallocationCallback mDeallocationCallback = {};

    public:
        inline const auto Capacity() const { return mCapacity; }
        inline const auto PaddedElementSize() const { return mPaddedElementSize; }
        inline const auto NonPaddedElementSize() const { return mNonPaddedElementSize; }
        inline const auto PerElementAlignment() const { return mPerElementAlignment; }
    };

}

#include "BufferCPUAccessor.inl"

