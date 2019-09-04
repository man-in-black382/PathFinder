#pragma once

#include "ShaderExport.hpp"

namespace HAL
{

    class RayTracingHitGroup
    {
    public:
        RayTracingHitGroup(const ShaderExport* closestHit, const ShaderExport* anyHit, const ShaderExport* intersection);

        void SetExportName(const std::wstring& name);

    private:
        std::wstring mName;

        const ShaderExport* mClosestHitExport;
        const ShaderExport* mAnyHitExport;
        const ShaderExport* mIntersectionExport;

        D3D12_HIT_GROUP_DESC mHitGroup{};

    public:
         const D3D12_HIT_GROUP_DESC& D3DHitGroup() const { return mHitGroup; }
         const std::wstring& ExportName() const { return mName; }
         const ShaderExport* ClosestHitExport() const { return mClosestHitExport; }
         const ShaderExport* AnyHitExport() const { return mAnyHitExport; }
         const ShaderExport* IntersectionExport() const { return mIntersectionExport; }
    };

}

