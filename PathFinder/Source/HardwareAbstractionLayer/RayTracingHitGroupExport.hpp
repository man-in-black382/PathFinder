#pragma once

#include "LibraryExport.hpp"

namespace HAL
{

    class RayTracingHitGroupExport
    {
    public:
        RayTracingHitGroupExport(const LibraryExport* closestHit, const LibraryExport* anyHit, const LibraryExport* intersection);

        void SetExportName(const std::wstring& name);

    private:
        std::wstring mName;

        const LibraryExport* mClosestHitExport;
        const LibraryExport* mAnyHitExport;
        const LibraryExport* mIntersectionExport;

        D3D12_HIT_GROUP_DESC mHitGroup{};

    public:
         const D3D12_HIT_GROUP_DESC& D3DHitGroup() const { return mHitGroup; }
         const std::wstring& ExportName() const { return mName; }
         const LibraryExport* ClosestHitExport() const { return mClosestHitExport; }
         const LibraryExport* AnyHitExport() const { return mAnyHitExport; }
         const LibraryExport* IntersectionExport() const { return mIntersectionExport; }
    };

}

