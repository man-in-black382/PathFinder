#pragma once

#include <unordered_map>

#include <aftermath/AftermathHelpers.hpp>

#include <HardwareAbstractionLayer/Shader.hpp>

namespace PathFinder
{
    class AftermathShaderDatabase
    {
    public:
        struct Binary
        {
            const uint8_t* Bytecode = nullptr;
            uint64_t Length = 0;
        };

        void AddShader(const HAL::Shader& shader);
        void AddLibrary(const HAL::Library& shader);

        const Binary* FindShaderBinary(const GFSDK_Aftermath_ShaderHash& shaderHash) const;
        const Binary* FindSourceShaderDebugData(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName) const;
        const Binary* FindShaderBinary(const GFSDK_Aftermath_ShaderInstructionsHash& shaderInstructionsHash) const;

    private:
        void AddCompiledObject(const Binary& binary, const Binary& pdb, const std::string& debugName);

        // List of shader binaries by ShaderHash.
        std::unordered_map<GFSDK_Aftermath_ShaderHash, Binary> mShaderBinaries;

        // Map from ShaderInstructionsHash to ShaderHash.
        std::unordered_map<GFSDK_Aftermath_ShaderInstructionsHash, GFSDK_Aftermath_ShaderHash> mShaderInstructionsToShaderHash;

        // List of available source shader debug information.
        std::unordered_map<GFSDK_Aftermath_ShaderDebugName, Binary> mSourceShaderDebugData;
    };
}
