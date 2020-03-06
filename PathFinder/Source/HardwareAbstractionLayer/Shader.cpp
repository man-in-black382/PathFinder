#include "Shader.hpp"
#include "Utils.h"

#include <d3dcompiler.h>
#include <dxcapi.h>

namespace HAL
{

    Shader::Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::wstring& entryPoint, Stage stage)
        : mBlob{ blob }, mEntryPoint{ entryPoint }, mStage{ stage } {}

}



