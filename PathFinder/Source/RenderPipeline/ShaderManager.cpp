#include "ShaderManager.hpp"
#include "../HardwareAbstractionLayer/ShaderCompiler.hpp"

namespace PathFinder
{

    ShaderManager::ShaderManager(const std::filesystem::path& shaderRootPath)
        : mShaderRootPath{ shaderRootPath } {}

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& vsFileName, const std::string& psFileName)
    {
        return {
            &GetShader(HAL::Shader::Stage::Vertex, vsFileName),
            &GetShader(HAL::Shader::Stage::Pixel, psFileName),
            nullptr, nullptr, nullptr, nullptr
        };
    }

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName)
    {
        return {
            &GetShader(HAL::Shader::Stage::Vertex, vsFileName),
            &GetShader(HAL::Shader::Stage::Pixel, psFileName),
            nullptr, nullptr,
            &GetShader(HAL::Shader::Stage::Geometry, gsFileName),
            nullptr
        };
    }

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& csFileName)
    {
        return { nullptr, nullptr, nullptr, nullptr, nullptr, &GetShader(HAL::Shader::Stage::Compute, csFileName) };
    }

    HAL::Shader& ShaderManager::GetShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath)
    {
        auto fullPath = mShaderRootPath / relativePath;
        HAL::Shader* shader = FindCachedShader(pipelineStage, fullPath);
        return shader ? *shader : LoadAndCacheShader(pipelineStage, fullPath);
    }

    HAL::Shader* ShaderManager::FindCachedShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& fullFilePath)
    {
        auto& internalMap = mShaderCache[pipelineStage];
        auto shaderIt = internalMap.find(fullFilePath.string());
        return (shaderIt == internalMap.end()) ? nullptr : &shaderIt->second;
    }

    HAL::Shader& ShaderManager::LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& fullFilePath)
    {
        mShaderCache[pipelineStage].emplace(fullFilePath.string(), mCompiler.Compile(fullFilePath, pipelineStage));
        return mShaderCache[pipelineStage].at(fullFilePath.string());
    }

}