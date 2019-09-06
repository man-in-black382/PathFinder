#pragma once

#include "ShaderExport.hpp"
#include "RootSignature.hpp"

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

    private:
        ShaderExport mExport;
        D3D12_DXIL_LIBRARY_DESC mLibrary{};
        const RootSignature* mLocalRootSignature;

    public:
        const D3D12_DXIL_LIBRARY_DESC& D3DLibrary() const { return mLibrary; }
        const ShaderExport& Export() const { return mExport; }
        const RootSignature* LocalRootSignature() const { return mLocalRootSignature; }
    };

}

