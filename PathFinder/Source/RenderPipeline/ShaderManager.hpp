#pragma once

#include "IShaderManager.hpp"
#include "../HardwareAbstractionLayer/Shader.hpp"

#include <unordered_map>
#include <filesystem>

namespace PathFinder
{

    class ShaderManager : public IShaderManager
    {
    public:
        ShaderManager(const std::filesystem::path& shaderRootPath);

        virtual ShaderBundle LoadShaders(const std::string& vsFileName, const std::string& psFileName) override;
        virtual ShaderBundle LoadShaders(const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName) override;
        virtual ShaderBundle LoadShaders(const std::string& csFileName) override;

    private:
        std::string ConstructFullShaderPath(const std::string& relativePath);
        HAL::Shader& GetShader(HAL::Shader::PipelineStage pipelineStage, const std::string& relativePath);
        HAL::Shader* FindCachedShader(HAL::Shader::PipelineStage pipelineStage, const std::string& fullFilePath);
        HAL::Shader& LoadAndCacheShader(HAL::Shader::PipelineStage pipelineStage, const std::string& fullFilePath);

        std::filesystem::path mShaderRootPath;
        std::unordered_map<HAL::Shader::PipelineStage, std::unordered_map<std::string, HAL::Shader>> mShaderCache;
    };

}
