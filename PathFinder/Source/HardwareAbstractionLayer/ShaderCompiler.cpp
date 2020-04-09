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

    ShaderCompiler::CompilationResult ShaderCompiler::Compile(const std::filesystem::path& path, Shader::Stage stage, const std::string& entryPoint, bool debugBuild)
    {
        assert_format(std::filesystem::exists(path), "Shader file ", path.filename(), " doesn't exist");

        std::wstring wEntryPoint = StringToWString(entryPoint);
        std::wstring wProfile = ProfileString(stage, Shader::Profile::P6_3);

        std::vector<std::wstring> arguments;
        arguments.push_back(L"/all_resources_bound");

        if (debugBuild)
        {
            arguments.push_back(L"/Zi");
            arguments.push_back(L"/Od");
        }
        
        std::vector<LPCWSTR> argumentPtrs;

        for (auto& argument : arguments)
        {
            argumentPtrs.push_back(argument.c_str());
        }

        ShaderFileReader reader{ path.parent_path(), mLibrary.Get() };

        Microsoft::WRL::ComPtr<IDxcBlob> source;
        Microsoft::WRL::ComPtr<IDxcOperationResult> result;

        reader.LoadSource(path.filename().wstring().c_str(), source.GetAddressOf());
  
        mCompiler->Compile(
            source.Get(),                       // program text
            path.filename().wstring().c_str(),  // file name, mostly for error messages
            wEntryPoint.c_str(),          // entry point function
            wProfile.c_str(),             // target profile
            argumentPtrs.data(),                // compilation arguments
            argumentPtrs.size(),                // number of compilation arguments
            nullptr, 0,                         // name/value defines and their count
            &reader,                            // handler for #include directives
            result.GetAddressOf()); 

        HRESULT hrCompilation{};
        result->GetStatus(&hrCompilation);

        if (SUCCEEDED(hrCompilation))
        {
            Microsoft::WRL::ComPtr<IDxcBlob> resultingBlob;
            result->GetResult(resultingBlob.GetAddressOf()); 

            CompilationResult compilationResult{ Shader{ resultingBlob, wEntryPoint, stage }, reader.AllReadFileRelativePaths() };
            return compilationResult;
        }
        else {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob;
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob16;

            result->GetErrorBuffer(printBlob.GetAddressOf());
            // We can use the library to get our preferred encoding.
            mLibrary->GetBlobAsUtf16(printBlob.Get(), printBlob16.GetAddressOf());
            OutputDebugStringW((LPWSTR)printBlob16->GetBufferPointer());

            CompilationResult compilationResult{ Shader{ nullptr, L"", stage }, {} };
            return compilationResult;
        }
    }

    std::wstring ShaderCompiler::ProfileString(Shader::Stage stage, Shader::Profile profile)
    {
        std::wstring profileString;

        switch (stage)
        {
        case Shader::Stage::Vertex: profileString = L"vs_"; break;
        case Shader::Stage::Hull: profileString = L"hs_"; break;
        case Shader::Stage::Domain:	profileString = L"ds_"; break;
        case Shader::Stage::Geometry: profileString = L"gs_"; break;
        case Shader::Stage::Pixel: profileString = L"ps_"; break;
        case Shader::Stage::Compute: profileString = L"cs_"; break;

        case Shader::Stage::RayGeneration: profileString = L"lib_"; break;
        case Shader::Stage::RayClosestHit: profileString = L"lib_"; break;
        case Shader::Stage::RayAnyHit: profileString = L"lib_"; break;
        case Shader::Stage::RayMiss: profileString = L"lib_"; break;
        case Shader::Stage::RayIntersection: profileString = L"lib_"; break;

        default: break;
        }

        switch (profile)
        {
        case Shader::Profile::P6_3: profileString += L"6_3"; break;
        case Shader::Profile::P6_4: profileString += L"6_4"; break;
        }

        return profileString;
    }

}

