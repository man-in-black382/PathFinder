#include "ShaderManager.hpp"

namespace PathFinder
{

    ShaderManager::ShaderManager(const std::filesystem::path& shaderRootPath)
        : mShaderRootPath{ shaderRootPath } {}

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& vsFileName, const std::string& psFileName)
    {
        return {
            GetShader(HAL::Shader::PipelineStage::Vertex, vsFileName),
            GetShader(HAL::Shader::PipelineStage::Pixel, psFileName),
        };
    }

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName)
    {
        return {
            GetShader(HAL::Shader::PipelineStage::Vertex, vsFileName),
            GetShader(HAL::Shader::PipelineStage::Pixel, psFileName),
        };
    }

    HAL::ShaderBundle ShaderManager::LoadShaders(const std::string& csFileName)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    std::string ShaderManager::ConstructFullShaderPath(const std::string& relativePath)
    {
        return mShaderRootPath.string() + relativePath;
    }

    HAL::Shader& ShaderManager::GetShader(HAL::Shader::PipelineStage pipelineStage, const std::string& relativePath)
    {
        std::string fullPath = ConstructFullShaderPath(relativePath);
        HAL::Shader* shader = FindCachedShader(pipelineStage, fullVSPath);
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
    }

}