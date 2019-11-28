#include "DXILLibrary.hpp"

#include <d3d12.h>

namespace HAL
{

    DXILLibrary::DXILLibrary(const ShaderExport& shaderExport)
        : mExport{ shaderExport }, mConfig{ 1, 1 }
    {
        mLibrary.DXILLibrary = shaderExport.AssosiatedShader()->D3DBytecode();
        mLibrary.NumExports = 1;
        mLibrary.pExports = &mExport.D3DExport();
    }

    DXILLibrary::DXILLibrary(const DXILLibrary& that) 
        : mExport{ that.mExport }, mConfig{ that.mConfig }
    {
        *this = that;
    }

    DXILLibrary::DXILLibrary(DXILLibrary&& that)
        : mExport{ std::move(that.mExport) }, mConfig{ std::move(that.mConfig) }
    {
        *this = std::move(that);
    }

    DXILLibrary& DXILLibrary::operator=(DXILLibrary&& that)
    {
        mExport = std::move(that.mExport);
        mLibrary = std::move(that.mLibrary);
        mLibrary.pExports = &mExport.D3DExport();
        mConfig = std::move(that.mConfig);
        return *this;
    }

    DXILLibrary& DXILLibrary::operator=(const DXILLibrary& that)
    {
        mExport = that.mExport;
        mLibrary = that.mLibrary;
        mLibrary.pExports = &mExport.D3DExport();
        mConfig = that.mConfig;
        return *this;
    }

    void DXILLibrary::SetLocalRootSignature(const RootSignature* signature)
    {
        mLocalRootSignature = signature;
    }

    void DXILLibrary::SetConfig(const RayTracingShaderConfig& config)
    {
        mConfig = config;
    }

}
