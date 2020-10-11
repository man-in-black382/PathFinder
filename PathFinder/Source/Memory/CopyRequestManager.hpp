#pragma once

#include <functional>

#include <HardwareAbstractionLayer/CommandList.hpp>
#include <HardwareAbstractionLayer/Resource.hpp>

namespace Memory
{

    class CopyRequestManager
    {
    public:
        using CopyCommand = std::function<void(HAL::CopyCommandListBase&)>;

        struct CopyRequest
        {
            const HAL::Resource* Resource = nullptr;
            CopyCommand Command;
        };

        void RequestUpload(const HAL::Resource* resource, const CopyCommand& copyCommand);
        void RequestReadback(const HAL::Resource* resource, const CopyCommand& copyCommand);

        void FlushUploadRequests();
        void FlushReadbackRequests();
        void FlushAllRequests();

    private:
        std::vector<CopyRequest> mUploadRequests;
        std::vector<CopyRequest> mReadbackRequests;

    public:
        inline const auto& UploadRequests() const { return mUploadRequests; }
        inline const auto& ReadbackRequests() const { return mReadbackRequests; }
    };

}
