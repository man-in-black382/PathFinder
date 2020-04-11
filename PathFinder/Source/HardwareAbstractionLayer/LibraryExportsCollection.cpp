#include "LibraryExportsCollection.hpp"

#include <d3d12.h>

namespace HAL
{

    LibraryExportsCollection::LibraryExportsCollection(const Library* library)
        : mLibrary{ library }
    {
        mCollection.DXILLibrary = library->D3DBytecode();
        mCollection.NumExports = 0;
        mCollection.pExports = nullptr;
    }

    void LibraryExportsCollection::SetLibrary(const Library* library)
    {
        mLibrary = library;
        mCollection.DXILLibrary = library->D3DBytecode();
    }

    void LibraryExportsCollection::AddExport(const LibraryExport& libraryExport)
    {
        mExports.push_back(libraryExport);
    }

    const D3D12_DXIL_LIBRARY_DESC& LibraryExportsCollection::GetD3DLibraryExports() const
    {
        mD3DExports.clear();

        for (const LibraryExport& libExport : mExports)
        {
            mD3DExports.push_back(libExport.D3DExport());
        }

        mCollection.NumExports = mD3DExports.size();
        mCollection.pExports = mD3DExports.data();

        return mCollection;
    }

}
