#include "ShaderCompiler.hpp"
#include "Utils.h"

#include "../Foundation/StringUtils.hpp"
#include "../Foundation/Assert.hpp"

#include <d3dcompiler.h>
#include <filewatch/FileWatcher.h>

namespace HAL
{

    ShaderFileReader::ShaderFileReader(const std::filesystem::path& rootPath, IDxcLibrary* library)
        : mRootPath{ rootPath }, mLibrary{ library } {}

    HRESULT STDMETHODCALLTYPE ShaderFileReader::LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource)
    {
        std::filesystem::path includePath{ pFilename };

        std::string fileName = includePath.string();

        // Search for the substring in string
        size_t pos = fileName.find("./");

        if (pos != std::string::npos)
        {
            // If found then erase it from string
            fileName.erase(pos, 2);
        }

        mReadFileList.push_back(fileName);

        includePath = mRootPath / pFilename;

        IDxcBlobEncoding* source;
        HRESULT result = mLibrary->CreateBlobFromFile(includePath.wstring().c_str(), nullptr, &source);
        *ppIncludeSource = source;
        return result;
    }

    // Implementing IUnknown
    // https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus
    //
    HRESULT STDMETHODCALLTYPE ShaderFileReader::QueryInterface(REFIID riid, void **ppvObject)
    {
        // Always set out parameter to NULL, validating it first.
        if (!ppvObject) return E_INVALIDARG;

        *ppvObject = NULL;
        if (riid == IID_IUnknown)
        {
            // Increment the reference count and return the pointer.
            *ppvObject = (LPVOID)this;
            AddRef();
            return NOERROR;
        }

        return E_NOINTERFACE;
    }

    ULONG ShaderFileReader::AddRef()
    {
        InterlockedIncrement(&mRefCount);
        return mRefCount;
    }

    ULONG ShaderFileReader::Release()
    {
        ULONG ulRefCount = InterlockedDecrement(&mRefCount);
        if (mRefCount == 0) delete this;
        return ulRefCount;
    }



    ShaderCompiler::ShaderCompiler()
    {
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(mLibrary.GetAddressOf())));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(mCompiler.GetAddressOf()))); 
    }

    ShaderCompiler::ShaderCompilationResult ShaderCompiler::CompileShader(const std::filesystem::path& path, Shader::Stage stage, const std::string& entryPoint, bool debugBuild)
    {
        BlobCompilationResult blobCompilationResult = CompileBlob(path, ProfileString(stage, Profile::P6_3), entryPoint, debugBuild);
        ShaderCompilationResult shaderCompilationResult{ Shader{ blobCompilationResult.Blob, blobCompilationResult.PDBBlob, entryPoint, stage }, blobCompilationResult.CompiledFileRelativePaths };
        shaderCompilationResult.CompiledShader.SetDebugName(blobCompilationResult.DebugName);
        return shaderCompilationResult;
    }

    ShaderCompiler::LibraryCompilationResult ShaderCompiler::CompileLibrary(const std::filesystem::path& path, bool debugBuild)
    {
        BlobCompilationResult blobCompilationResult = CompileBlob(path, LibProfileString(Profile::P6_3), "", debugBuild);
        LibraryCompilationResult libraryCompilationResult{ Library{ blobCompilationResult.Blob, blobCompilationResult.PDBBlob }, blobCompilationResult.CompiledFileRelativePaths };
        libraryCompilationResult.CompiledLibrary.SetDebugName(blobCompilationResult.DebugName);
        return libraryCompilationResult;
    }

    std::string ShaderCompiler::ProfileString(Shader::Stage stage, Profile profile)
    {
        std::string profileString;

        switch (stage)
        {
        case Shader::Stage::Vertex: profileString = "vs_"; break;
        case Shader::Stage::Hull: profileString = "hs_"; break;
        case Shader::Stage::Domain:	profileString = "ds_"; break;
        case Shader::Stage::Geometry: profileString = "gs_"; break;
        case Shader::Stage::Pixel: profileString = "ps_"; break;
        case Shader::Stage::Compute: profileString = "cs_"; break;
        default: break;
        }

        switch (profile)
        {
        case Profile::P6_3: profileString += "6_3"; break;
        case Profile::P6_4: profileString += "6_4"; break;
        default: break;
        }

        return profileString;
    }

    std::string ShaderCompiler::LibProfileString(Profile profile)
    {
        switch (profile)
        {
        case Profile::P6_3: return "lib_6_3";
        case Profile::P6_4: return "lib_6_4";
        default: return "lib_6_3";
        }
    }

    ShaderCompiler::BlobCompilationResult ShaderCompiler::CompileBlob(const std::filesystem::path& path, const std::string& profileString, const std::string& entryPoint, bool debugBuild)
    {
        assert_format(std::filesystem::exists(path), "Shader file ", path.filename(), " doesn't exist");

        std::wstring wEntryPoint = StringToWString(entryPoint);
        std::wstring wProfile = StringToWString(profileString);
        LPWSTR suggestedDebugName = nullptr;

        std::vector<std::wstring> arguments;
        arguments.push_back(L"/all_resources_bound");

        if (debugBuild)
        {
            arguments.push_back(L"/Zi");
            arguments.push_back(L"/Od");

            // Produce a smaller shader without duplicated debug information
            arguments.push_back(L"/Qstrip_debug"); 
        }

        std::vector<LPCWSTR> argumentPtrs;

        for (auto& argument : arguments)
        {
            argumentPtrs.push_back(argument.c_str());
        }

        ShaderFileReader reader{ path.parent_path(), mLibrary.Get() };

        Microsoft::WRL::ComPtr<IDxcBlob> sourceBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> pdbBlob;
        Microsoft::WRL::ComPtr<IDxcOperationResult> result;

        reader.LoadSource(path.filename().wstring().c_str(), sourceBlob.GetAddressOf());

        // Note: when compiling libraries, entry point and profile are ignored

        if (debugBuild)
        {
            mCompiler->CompileWithDebug(
                sourceBlob.Get(),                   // Program text
                path.filename().wstring().c_str(),  // File name, mostly for error messages
                wEntryPoint.c_str(),                // Entry point function
                wProfile.c_str(),                   // Target profile
                argumentPtrs.data(),                // Compilation arguments
                argumentPtrs.size(),                // Number of compilation arguments
                nullptr, 0,                         // Name/value defines and their count
                &reader,                            // Handler for #include directives
                result.GetAddressOf(),              // Compiler output status, buffer, and errors
                &suggestedDebugName,                            // Suggested file name for debug blob.
                pdbBlob.GetAddressOf());            // Debug info blob
        }
        else
        {
            mCompiler->Compile(
                sourceBlob.Get(),                   // Program text
                path.filename().wstring().c_str(),  // File name, mostly for error messages
                wEntryPoint.c_str(),                // Entry point function
                wProfile.c_str(),                   // Target profile
                argumentPtrs.data(),                // Compilation arguments
                argumentPtrs.size(),                // Number of compilation arguments
                nullptr, 0,                         // Name/value defines and their count
                &reader,                            // Handler for #include directives
                result.GetAddressOf());
        }
        
        HRESULT hrCompilation{};
        result->GetStatus(&hrCompilation);

        if (SUCCEEDED(hrCompilation))
        {
            Microsoft::WRL::ComPtr<IDxcBlob> compiledShaderBlob;
            result->GetResult(compiledShaderBlob.GetAddressOf());
            std::wstring pdbAutoGeneratedFileName = suggestedDebugName ? suggestedDebugName : L"";

            return { compiledShaderBlob, pdbBlob, reader.AllReadFileRelativePaths(), WStringToString(pdbAutoGeneratedFileName) };
        }
        else {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob;
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob16;

            result->GetErrorBuffer(printBlob.GetAddressOf());
            // We can use the library to get our preferred encoding.
            mLibrary->GetBlobAsUtf16(printBlob.Get(), printBlob16.GetAddressOf());
            OutputDebugStringW((LPWSTR)printBlob16->GetBufferPointer());

            return { nullptr, nullptr, {}, "" };
        }
    }

}

