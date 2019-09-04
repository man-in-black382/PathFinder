#pragma once

#include "IShaderManager.hpp"
#include "../HardwareAbstractionLayer/Shader.hpp"
#include "../HardwareAbstractionLayer/ShaderCompiler.hpp"

#include <unordered_map>
#include <filesystem>

namespace PathFinder
{

    class ShaderManager : public IShaderManager
    {
    public:
        ShaderManager(const std::filesystem::path& shaderRootPath);

        virtual HAL::ShaderBundle LoadShaders(const std::string& vsFileName, const std::string& psFileName) override;
        virtual HAL::ShaderBundle LoadShaders(const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName) override;
        virtual HAL::ShaderBundle LoadShaders(const std::string& csFileName) override;

    private:
        HAL::Shader& GetShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);
        HAL::Shader* FindCachedShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& fullFilePath);
        HAL::Shader& LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& fullFilePath);

        HAL::ShaderCompiler mCompiler;
        std::filesystem::path mShaderRootPath;
        std::unordered_map<HAL::Shader::Stage, std::unordered_map<std::string, HAL::Shader>> mShaderCache;
    };

}
