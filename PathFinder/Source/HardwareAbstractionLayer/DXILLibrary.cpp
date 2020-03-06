#include "DXILLibrary.hpp"

#include <d3d12.h>

namespace HAL
{

    DXILLibrary::DXILLibrary(const ShaderExport& shaderExport)
        : mExport{ shaderExport }
    {
        mLibrary.DXILLibrary = shaderExport.AssosiatedShader()->D3DBytecode();
        mLibrary.NumExports = 1;
        mLibrary.pExports = &mExport.D3DExport();
    }

    DXILLibrary::DXILLibrary(const DXILLibrary& that) 
        : mExport{ that.mExport }, mLibrary{ that.mLibrary }
    {
        mLibrary.pExports = &mExport.D3DExport();
    }

    DXILLibrary::DXILLibrary(DXILLibrary&& that)
        : mExport{ std::move(that.mExport) }, mLibrary{ std::move(that.mLibrary) }
    {
        mLibrary.pExports = &mExport.D3DExport();
    }

    DXILLibrary& DXILLibrary::operator=(DXILLibrary&& that)
    {
        mExport = std::move(that.mExport);
        mLibrary = std::move(that.mLibrary);
        mLibrary.pExports = &mExport.D3DExport();
        return *this;
    }

    DXILLibrary& DXILLibrary::operator=(const DXILLibrary& that)
    {
        mExport = that.mExport;
        mLibrary = that.mLibrary;
        mLibrary.pExports = &mExport.D3DExport();
        return *this;
    }

    void DXILLibrary::SetLocalRootSignature(const RootSignature* signature)
    {
        mLocalRootSignature = signature;
    }

}
