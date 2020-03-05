#pragma once

#include "ShaderExport.hpp"
#include "RootSignature.hpp"

namespace HAL
{

    class RayTracingHitGroup
    {
    public:
        RayTracingHitGroup(const ShaderExport* closestHit, const ShaderExport* anyHit, const ShaderExport* intersection, const RootSignature* localSignature = nullptr);

        void SetExportName(const std::wstring& name);

    private:
        std::wstring mName;

        const ShaderExport* mClosestHitExport;
        const ShaderExport* mAnyHitExport;
        const ShaderExport* mIntersectionExport;
        const RootSignature* mLocalRootSignature = nullptr;

        D3D12_HIT_GROUP_DESC mHitGroup{};

    public:
         const D3D12_HIT_GROUP_DESC& D3DHitGroup() const { return mHitGroup; }
         const std::wstring& ExportName() const { return mName; }
         const ShaderExport* ClosestHitExport() const { return mClosestHitExport; }
         const ShaderExport* AnyHitExport() const { return mAnyHitExport; }
         const ShaderExport* IntersectionExport() const { return mIntersectionExport; }
         const RootSignature* LocalRootSignature() const { return mLocalRootSignature; }
    };

}

