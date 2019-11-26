#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>

#include "Shader.hpp"

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
        struct CompilationResult
        {
            Shader CompiledShader;
            std::vector<std::string> CompiledFileRelativePaths;
        };

        ShaderCompiler();

        CompilationResult Compile(const std::filesystem::path& path, Shader::Stage stage, bool debugBuild);

    private:
        struct CompilerInputs
        {
            CompilerInputs(Shader::Stage stage, Shader::Profile profile);

            std::wstring EntryPoint;
            std::wstring Profile;
        };

        Microsoft::WRL::ComPtr<IDxcLibrary> mLibrary;
        Microsoft::WRL::ComPtr<IDxcCompiler> mCompiler;
    };

}

