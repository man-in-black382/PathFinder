#pragma once

#include "LibraryExport.hpp"
#include "RootSignature.hpp"
#include "RayTracingShaderConfig.hpp"

namespace HAL
{

    class LibraryExportsCollection
    {
    public:
        LibraryExportsCollection(const Library* library);

        void SetLibrary(const Library* library);
        void AddExport(const LibraryExport& libraryExport);
        const D3D12_DXIL_LIBRARY_DESC& GetD3DLibraryExports() const;

    private:
        const Library* mLibrary = nullptr;
        std::vector<LibraryExport> mExports;

        // Not part of the external state as these are merely caches
        mutable std::vector<D3D12_EXPORT_DESC> mD3DExports;
        mutable D3D12_DXIL_LIBRARY_DESC mCollection{};
    };

}

