namespace Memory
{

    template <class T>
    T* GPUResource::Write()
    {
        if (!mCompletedUploadBuffer)
        {
            return nullptr;
        }

        // No need to unmap as upload buffers can be mapped persistently
        return reinterpret_cast<T*>(mCompletedUploadBuffer->Map());
    }

    template <class T>
    void GPUResource::Read(const ReadbackSession<T>& session) const
    {
        assert_format(mUploadStrategy != UploadStrategy::DirectAccess, "DirectAccess upload resource does not support reads");

        if (!mCompletedReadbackBuffer)
        {
            session(nullptr);
            return;
        }

        const T* mappedMemory = reinterpret_cast<T*>(mCompletedReadbackBuffer->Map());
        session(mappedMemory);
        mBuffer->Unmap(); // Invalidate CPU cache before next read
    }

}
