#include "CopyRequestManager.hpp"



namespace Memory
{

    void CopyRequestManager::RequestUpload(const HAL::Resource* resource, const CopyCommand& copyCommand)
    {
        mUploadRequests.emplace_back(CopyRequest{ resource, copyCommand });
    }

    void CopyRequestManager::RequestReadback(const HAL::Resource* resource, const CopyCommand& copyCommand)
    {
        mReadbackRequests.emplace_back(CopyRequest{ resource, copyCommand });
    }

    void CopyRequestManager::FlushUploadRequests()
    {
        mUploadRequests.clear();
    }

    void CopyRequestManager::FlushReadbackRequests()
    {
        mReadbackRequests.clear();
    }

    void CopyRequestManager::FlushAllRequests()
    {
        FlushUploadRequests();
        FlushReadbackRequests();
    }

}

