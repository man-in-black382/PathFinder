#include "ShaderManager.hpp"

namespace PathFinder
{

    ShaderManager::ShaderManager(const std::filesystem::path& shaderRootPath)
        : mShaderRootPath{ shaderRootPath } {}

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& vsFileName, const std::string& psFileName)
    {
        return {
            &GetShader(HAL::Shader::PipelineStage::Vertex, vsFileName),
            &GetShader(HAL::Shader::PipelineStage::Pixel, psFileName),
            nullptr, nullptr, nullptr, nullptr
        };
    }

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName)
    {
        return {
            &GetShader(HAL::Shader::PipelineStage::Vertex, vsFileName),
            &GetShader(HAL::Shader::PipelineStage::Pixel, psFileName),
            nullptr, nullptr,
            &GetShader(HAL::Shader::PipelineStage::Geometry, gsFileName),
            nullptr
        };
    }

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& csFileName)
    {
        return { nullptr, nullptr, nullptr, nullptr, nullptr, &GetShader(HAL::Shader::PipelineStage::Compute, csFileName) };
    }

    std::string ShaderManager::ConstructFullShaderPath(const std::string& relativePath)
    {
        return mShaderRootPath.string() + relativePath;
    }

    HAL::Shader& ShaderManager::GetShader(HAL::Shader::PipelineStage pipelineStage, const std::string& relativePath)
    {
        std::string fullPath = ConstructFullShaderPath(relativePath);
        HAL::Shader* shader = FindCachedShader(pipelineStage, fullPath);
        return shader ? *shader : LoadAndCacheShader(pipelineStage, fullPath);
    }

    HAL::Shader* ShaderManager::FindCachedShader(HAL::Shader::PipelineStage pipelineStage, const std::string& fullFilePath)
    {
        auto& internalMap = mShaderCache[pipelineStage];
        auto shaderIt = internalMap.find(fullFilePath);
        return (shaderIt == internalMap.end()) ? nullptr : &shaderIt->second;
    }

    HAL::Shader& ShaderManager::LoadAndCacheShader(HAL::Shader::PipelineStage pipelineStage, const std::string& fullFilePath)
    {
        mShaderCache[pipelineStage].emplace(fullFilePath, HAL::Shader{ fullFilePath, pipelineStage });
        return mShaderCache[pipelineStage].at(fullFilePath);
    }

}