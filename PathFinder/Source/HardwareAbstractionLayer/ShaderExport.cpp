#include "DXILLibrary.hpp"

#include <d3d12.h>

namespace HAL
{

    ShaderExport::ShaderExport(const Shader* shader)
        : mShader{ shader }
    {
        mExport.Flags = D3D12_EXPORT_FLAG_NONE;
        mExport.ExportToRename = mShader->EntryPoint().c_str();
        mExport.Name = mExport.ExportToRename;
    }

    ShaderExport::ShaderExport(const ShaderExport& that)
    {
        *this = that;
    }

    ShaderExport::ShaderExport(ShaderExport&& that)
    {
        *this = std::move(that);
    }

    HAL::ShaderExport::ShaderExport& ShaderExport::operator=(const ShaderExport& that)
    {
        mExport = that.mExport;
        mName = that.mName;
        mShader = that.mShader;
        mExport.ExportToRename = mShader->EntryPoint().c_str();
        mExport.Name = mName.c_str();
        return *this;
    }

    HAL::ShaderExport::ShaderExport& ShaderExport::operator=(ShaderExport&& that)
    {
        mExport = std::move(that.mExport);
        mName = std::move(that.mName);
        mShader = std::move(that.mShader);
        mExport.ExportToRename = mShader->EntryPoint().c_str();
        mExport.Name = mName.c_str();
        return *this;
    }

    void ShaderExport::SetExportName(const std::wstring& name)
    {
        mName = name;
        mExport.Name = mName.c_str();
    }

}
