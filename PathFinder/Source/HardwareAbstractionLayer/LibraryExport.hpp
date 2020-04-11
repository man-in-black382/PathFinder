#pragma once

#include "Shader.hpp"

namespace HAL
{

    class LibraryExport
    {
    public:
        LibraryExport(const std::string& originalExportName);
        LibraryExport(const LibraryExport& that);
        LibraryExport(LibraryExport&& that);
        ~LibraryExport() = default;

        LibraryExport& operator=(const LibraryExport& that);
        LibraryExport& operator=(LibraryExport&& that);

        void SetExportName(const std::string& name);

    private:
        // External export name 
        std::wstring mExportName;

        // Internal export name, such as Shader Entry Point name
        std::wstring mExportToRename;

        D3D12_EXPORT_DESC mExport{};

    public:
        D3D12_EXPORT_DESC& D3DExport() { return mExport; }
        const D3D12_EXPORT_DESC& D3DExport() const { return mExport; }
        const std::wstring& ExportName() const { return mExportName; }
    };

}

