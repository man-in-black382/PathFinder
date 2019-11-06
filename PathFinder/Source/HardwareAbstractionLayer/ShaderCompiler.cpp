#include "ShaderCompiler.hpp"
#include "Utils.h"

#include <d3dcompiler.h>

namespace HAL
{

    ShaderFileReader::ShaderFileReader(const std::filesystem::path& rootPath, IDxcLibrary* library)
        : mRootPath{ rootPath }, mLibrary{ library } {}

    HRESULT STDMETHODCALLTYPE ShaderFileReader::LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource)
    {
        auto includePath = mRootPath / pFilename;
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

    Shader ShaderCompiler::Compile(const std::filesystem::path& path, Shader::Stage stage)
    {
        CompilerInputs inputs{ stage, Shader::Profile::P6_3 };

        LPCWSTR arguments[] = { L"/Zi", L"/Od" }; // Provide debug info, disable optimization 

        ShaderFileReader reader{ path.parent_path(), mLibrary.Get() };

        Microsoft::WRL::ComPtr<IDxcBlob> source;
        Microsoft::WRL::ComPtr<IDxcOperationResult> result;

        reader.LoadSource(path.filename().wstring().c_str(), source.GetAddressOf());

        mCompiler->Compile(
            source.Get(),                       // program text
            path.filename().wstring().c_str(),  // file name, mostly for error messages
            inputs.EntryPoint.c_str(),          // entry point function
            inputs.Profile.c_str(),             // target profile
            arguments,                          // compilation arguments
            _countof(arguments),                // number of compilation arguments
            nullptr, 0,                         // name/value defines and their count
            &reader,                            // handler for #include directives
            result.GetAddressOf());

        HRESULT hrCompilation;
        result->GetStatus(&hrCompilation);

        if (SUCCEEDED(hrCompilation))
        {
            Microsoft::WRL::ComPtr<IDxcBlob> resultingBlob;
            result->GetResult(resultingBlob.GetAddressOf()); 
            return Shader{ resultingBlob, inputs.EntryPoint, stage }; 
        }
        else {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob;
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob16;

            result->GetErrorBuffer(printBlob.GetAddressOf());
            // We can use the library to get our preferred encoding.
            mLibrary->GetBlobAsUtf16(printBlob.Get(), printBlob16.GetAddressOf());
            OutputDebugStringW((LPWSTR)printBlob16->GetBufferPointer());
            return Shader{ nullptr, L"", stage }; 
        }
    }

    ShaderCompiler::CompilerInputs::CompilerInputs(Shader::Stage stage, Shader::Profile profile)
    {
        std::wstring profilePrefix;

        switch (stage) 
        {
        case Shader::Stage::Vertex:		EntryPoint = L"VSMain"; profilePrefix = L"vs_"; break;
        case Shader::Stage::Hull:		EntryPoint = L"HSMain"; profilePrefix = L"hs_"; break;
        case Shader::Stage::Domain:		EntryPoint = L"DSMain"; profilePrefix = L"ds_"; break;
        case Shader::Stage::Geometry:	EntryPoint = L"GSMain"; profilePrefix = L"gs_"; break;
        case Shader::Stage::Pixel:		EntryPoint = L"PSMain"; profilePrefix = L"ps_"; break;
        case Shader::Stage::Compute:	EntryPoint = L"CSMain"; profilePrefix = L"cs_"; break;

        case Shader::Stage::RayGeneration:      EntryPoint = L"RayGeneration";   profilePrefix = L"lib_"; break;
        case Shader::Stage::RayClosestHit:      EntryPoint = L"RayClosestHit";   profilePrefix = L"lib_"; break;
        case Shader::Stage::RayAnyHit:          EntryPoint = L"RayAnyHit";       profilePrefix = L"lib_"; break;
        case Shader::Stage::RayMiss:            EntryPoint = L"RayMiss";         profilePrefix = L"lib_"; break;
        case Shader::Stage::RayIntersection:    EntryPoint = L"RayIntersection"; profilePrefix = L"lib_"; break;

        default: break;
        }

        switch (profile)
        {
        case Shader::Profile::P5_1: Profile = profilePrefix + L"5_1"; break;
        case Shader::Profile::P6_3: Profile = profilePrefix + L"6_3"; break;
        }
    }

}

