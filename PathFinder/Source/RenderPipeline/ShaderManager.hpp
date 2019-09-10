#pragma once

#include "../HardwareAbstractionLayer/Shader.hpp"
#include "../HardwareAbstractionLayer/ShaderCompiler.hpp"

#include "ShaderFileNames.hpp"

#include <unordered_map>
#include <filesystem>

namespace PathFinder
{

    class ShaderManager
    {
    public:
        ShaderManager(const std::filesystem::path& shaderRootPath);

        HAL::GraphicsShaderBundle LoadShaders(const GraphicsShaderFileNames& fileNames);
        HAL::ComputeShaderBundle LoadShaders(const ComputeShaderFileNames& fileNames);
        HAL::RayTracingShaderBundle LoadShaders(const RayTracingShaderFileNames& fileNames);

    private:
        HAL::Shader& GetShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);
        HAL::Shader* FindCachedShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& fullFilePath);
        HAL::Shader& LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& fullFilePath);

        HAL::ShaderCompiler mCompiler;
        std::filesystem::path mShaderRootPath;
        std::unordered_map<HAL::Shader::Stage, std::unordered_map<std::string, HAL::Shader>> mShaderCache;
    };

}
