#include "RayTracingHitGroup.hpp"

#include <d3d12.h>

namespace HAL
{

    RayTracingHitGroup::RayTracingHitGroup(const ShaderExport* closestHit, const ShaderExport* anyHit, const ShaderExport* intersection)
        : mClosestHitExport{ closestHit }, mAnyHitExport{ anyHit }, mIntersectionExport{ intersection }
    {
        if (closestHit)
        {
            mHitGroup.ClosestHitShaderImport = closestHit->ExportName().c_str();
            mName += closestHit->ExportName();
        }

        if (anyHit)
        {
            mHitGroup.AnyHitShaderImport = anyHit->ExportName().c_str();
            mName += anyHit->ExportName();
        }

        if (intersection) 
        {
            mHitGroup.IntersectionShaderImport = intersection->ExportName().c_str();
            mName += intersection->ExportName();
        }

        mHitGroup.HitGroupExport = mName.c_str();
        mHitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    }

    void RayTracingHitGroup::SetExportName(const std::wstring& name)
    {
        mName = name;
        mHitGroup.HitGroupExport = mName.c_str();
    }

}
