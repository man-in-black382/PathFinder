#pragma once

#include "ShaderExport.hpp"
#include "RootSignature.hpp"
#include "RayTracingShaderConfig.hpp"

namespace HAL
{

    class DXILLibrary
    {
    public:
        DXILLibrary(const ShaderExport& shaderExport);
        DXILLibrary(const DXILLibrary& that);
        DXILLibrary(DXILLibrary&& that);
        ~DXILLibrary() = default;

        DXILLibrary& operator=(const DXILLibrary& that);
        DXILLibrary& operator=(DXILLibrary&& that);

        void SetLocalRootSignature(const RootSignature* signature);
        void SetConfig(const RayTracingShaderConfig& config);

    private:
        ShaderExport mExport;
        D3D12_DXIL_LIBRARY_DESC mLibrary{};
        RayTracingShaderConfig mConfig;
        const RootSignature* mLocalRootSignature = nullptr;

    public:
        inline const D3D12_DXIL_LIBRARY_DESC& D3DLibrary() const { return mLibrary; }
        inline const ShaderExport& Export() const { return mExport; }
        inline const RootSignature* LocalRootSignature() const { return mLocalRootSignature; }
        inline const RayTracingShaderConfig& Config() const { return mConfig; }
    };

}

