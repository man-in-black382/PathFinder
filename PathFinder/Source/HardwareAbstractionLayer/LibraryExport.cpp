#include "LibraryExportsCollection.hpp"

#include <d3d12.h>

#include <Foundation/StringUtils.hpp>

namespace HAL
{

    LibraryExport::LibraryExport(const std::string& originalExportName)
        : mExportName{ StringToWString(originalExportName) }, mExportToRename{ StringToWString(originalExportName) }
    {
        mExport.Flags = D3D12_EXPORT_FLAG_NONE;
        mExport.ExportToRename = mExportToRename.c_str();
        mExport.Name = mExport.ExportToRename;
    }

    LibraryExport::LibraryExport(const LibraryExport& that)
    {
        *this = that;
    }

    LibraryExport::LibraryExport(LibraryExport&& that)
    {
        *this = std::move(that);
    }

    LibraryExport& LibraryExport::operator=(const LibraryExport& that)
    {
        mExport = that.mExport;
        mExportName = that.mExportName;
        mExportToRename = that.mExportToRename;
        mExport.ExportToRename = mExportToRename.c_str();
        mExport.Name = mExportName.c_str();
        return *this;
    }

    LibraryExport& LibraryExport::operator=(LibraryExport&& that)
    {
        mExport = std::move(that.mExport);
        mExportName = std::move(that.mExportName);
        mExportToRename = std::move(that.mExportToRename);
        mExport.ExportToRename = mExportToRename.c_str();
        mExport.Name = mExportName.c_str();
        return *this;
    }

    void LibraryExport::SetExportName(const std::string& name)
    {
        mExportName = StringToWString(name);
        mExport.Name = mExportName.c_str();
    }

}
