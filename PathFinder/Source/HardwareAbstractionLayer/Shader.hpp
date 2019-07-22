#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace HAL
{
    
    class Shader
    {
    public:
        enum class PipelineStage { Vertex, Hull, Domain, Geometry, Pixel, Compute };

        Shader(const std::wstring& filePath, PipelineStage pipelineStage);

    private:
        Microsoft::WRL::ComPtr<ID3DBlob> mBlob;
        void* mBytecode;
        uint64_t mBytecodeSize;

    public:
        inline auto D3DBytecode() const { return D3D12_SHADER_BYTECODE{ mBytecode, mBytecodeSize }; }
    };



    class ShaderBundle
    {
    public:
        ShaderBundle(const std::filesystem::path& shaderRootPath, const std::string& vsFileName, const std::string& psFileName);
        ShaderBundle(const std::filesystem::path& shaderRootPath, const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName);
        ShaderBundle(const std::filesystem::path& shaderRootPath, const std::string& csFileName);

    private:
        std::vector<Shader> mShaders;


    };

}

