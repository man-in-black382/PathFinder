#pragma once

#include "Shader.hpp"

namespace HAL
{

    class ShaderExport
    {
    public:
        ShaderExport(const Shader* shader);
        ShaderExport(const ShaderExport& that);
        ShaderExport(ShaderExport&& that);
        ~ShaderExport() = default;

        ShaderExport& operator=(const ShaderExport& that);
        ShaderExport& operator=(ShaderExport&& that);

        void SetExportName(const std::wstring& name);

    private:
        const Shader* mShader;
        std::wstring mName;
        D3D12_EXPORT_DESC mExport{};

    public:
        D3D12_EXPORT_DESC& D3DExport() { return mExport; }
        const D3D12_EXPORT_DESC& D3DExport() const { return mExport; }

        const Shader* AssosiatedShader() const { return mShader; }
        const std::wstring& ExportName() const { return mName; }
    };

}

