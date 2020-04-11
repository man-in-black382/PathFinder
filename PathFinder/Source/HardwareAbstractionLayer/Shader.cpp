#include "Shader.hpp"
#include "Utils.h"

#include "../Foundation/StringUtils.hpp"

#include <d3dcompiler.h>
#include <dxcapi.h>

namespace HAL
{

    Shader::Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::string& entryPoint, Stage stage)
        : mBlob{ blob }, mEntryPoint{ entryPoint }, mEntryPointName{ entryPoint }, mStage{ stage } {}

    Library::Library(const Microsoft::WRL::ComPtr<IDxcBlob>& blob)
        : mBlob{ blob } {}

}



