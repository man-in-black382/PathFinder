#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>

#include "Shader.hpp"
#include "LibraryExportsCollection.hpp"

#include <dxcapi.h>

namespace HAL
{
    /// Custom include handler. Adds more flexibility.
    struct ShaderFileReader : public IDxcIncludeHandler
    {
    public:
        ShaderFileReader(const std::filesystem::path& rootPath, IDxcLibrary* library);

        virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource) override;
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

        virtual ULONG AddRef() override;
        virtual ULONG Release() override;

    private:
        std::vector<std::string> mReadFileList;
        std::filesystem::path mRootPath;
        IDxcLibrary *mLibrary;
        ULONG mRefCount;

    public:
        inline const auto& AllReadFileRelativePaths() const { return mReadFileList; }
    };

    class ShaderCompiler
    {
    public:
        enum class Profile
        {
            P6_3, P6_4
        };

        struct ShaderCompilationResult
        {
            Shader CompiledShader;
            std::vector<std::string> CompiledFileRelativePaths;
        };

        struct LibraryCompilationResult
        {
            Library CompiledLibrary;
            std::vector<std::string> CompiledFileRelativePaths;
        };

        ShaderCompiler();

        ShaderCompilationResult CompileShader(const std::filesystem::path& path, Shader::Stage stage, const std::string& entryPoint, bool debugBuild, bool separatePDB);
        LibraryCompilationResult CompileLibrary(const std::filesystem::path& path, bool debugBuild, bool separatePDB);

    private:
        struct BlobCompilationResult
        {
            Microsoft::WRL::ComPtr<IDxcBlob> Blob;
            Microsoft::WRL::ComPtr<IDxcBlob> PDBBlob;
            std::vector<std::string> CompiledFileRelativePaths;
            std::string DebugName;
        };

        std::string ProfileString(Shader::Stage stage, Profile profile);
        std::string LibProfileString(Profile profile);
        BlobCompilationResult CompileBlob(const std::filesystem::path& path, const std::string& profileString, const std::string& entryPoint, bool debugBuild, bool separatePDB);

        Microsoft::WRL::ComPtr<IDxcLibrary> mLibrary;
        Microsoft::WRL::ComPtr<IDxcCompiler2> mCompiler;
    };

}

