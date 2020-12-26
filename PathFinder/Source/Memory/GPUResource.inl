#include <Foundation/MemoryUtils.hpp>


namespace Memory
{

    template <class T>
    T* GPUResource::WriteOnlyPtr()
    {
        if (!CurrentFrameUploadBuffer())
        {
            return nullptr;
        }

        // No need to unmap as upload buffers can be mapped persistently
        return reinterpret_cast<T*>(CurrentFrameUploadBuffer()->Map());
    }

    template <class T>
    void GPUResource::Write(const T* data, uint64_t startIndex, uint64_t objectCount, uint64_t objectAlignment)
    {
        uint64_t alignedObjectSizeInBytes = Foundation::MemoryUtils::Align(sizeof(T), objectAlignment);
        uint64_t copyRegionSizeInBytes = alignedObjectSizeInBytes * objectCount;
        uint64_t byteOffset = alignedObjectSizeInBytes * startIndex;

        uint8_t* writeOnlyPtr = WriteOnlyPtr<uint8_t>();

        assert_format(writeOnlyPtr, "Need to request a write operation before trying to write data to resource");

        memcpy(writeOnlyPtr + byteOffset, data, copyRegionSizeInBytes);
    }

    template <class T>
    void GPUResource::Read(const ReadbackSession<T>& session) const
    {
        assert_format(mAccessStrategy != AccessStrategy::DirectUpload, "DirectUpload resource does not support CPU reads");

        if (!mCompletedReadbackBuffer)
        {
            session(nullptr);
            return;
        }

        const T* mappedMemory = reinterpret_cast<T*>(mCompletedReadbackBuffer->Map());
        session(mappedMemory);
        mCompletedReadbackBuffer->Unmap(); // Invalidate CPU cache before next read
    }

}
