#include "Shader.hpp"
#include "Utils.h"

#include "../Foundation/StringUtils.hpp"

#include <d3dcompiler.h>
#include <dxcapi.h>

namespace HAL
{

    Shader::Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const Microsoft::WRL::ComPtr<IDxcBlob>& pdbBlob, const std::string& entryPoint, Stage stage)
        : mBlob{ blob }, mPDBBlob{ pdbBlob }, mEntryPoint{ entryPoint }, mEntryPointName{ entryPoint }, mStage{ stage } {}

    void Shader::SetDebugName(const std::string& name)
    {
        mDebugName = name;
    }



    Library::Library(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const Microsoft::WRL::ComPtr<IDxcBlob>& pdbBlob)
        : mBlob{ blob }, mPDBBlob{ pdbBlob } {}

    void Library::SetDebugName(const std::string& name)
    {
        mDebugName = name;
    }

}



