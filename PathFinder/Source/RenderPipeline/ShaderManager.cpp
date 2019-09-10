#include "ShaderManager.hpp"

namespace PathFinder
{

    ShaderManager::ShaderManager(const std::filesystem::path& shaderRootPath)
        : mShaderRootPath{ shaderRootPath } {}

    HAL::GraphicsShaderBundle ShaderManager::LoadShaders(const GraphicsShaderFileNames& fileNames)
    {
        return {
            &GetShader(HAL::Shader::Stage::Vertex, fileNames.VertexShaderFileName),
            &GetShader(HAL::Shader::Stage::Pixel, fileNames.PixelShaderFileName),
            nullptr, nullptr,
            fileNames.GeometryShaderFileName ? &GetShader(HAL::Shader::Stage::Geometry, *fileNames.GeometryShaderFileName) : nullptr
        };
    }

    HAL::ComputeShaderBundle ShaderManager::LoadShaders(const ComputeShaderFileNames& fileNames)
    {
        return { &GetShader(HAL::Shader::Stage::Compute, fileNames.ComputeShaderFileName) };
    }

    HAL::RayTracingShaderBundle ShaderManager::LoadShaders(const RayTracingShaderFileNames& fileNames)
    {
        return {
            &GetShader(HAL::Shader::Stage::RayGeneration, fileNames.RayGenShaderFileName),
            &GetShader(HAL::Shader::Stage::RayClosestHit, fileNames.ClosestHitShaderFileName),
            &GetShader(HAL::Shader::Stage::RayMiss, fileNames.MissShaderFileName),
            fileNames.AnyHitShaderFileName ? &GetShader(HAL::Shader::Stage::RayAnyHit, *fileNames.AnyHitShaderFileName) : nullptr,
            fileNames.IntersectionShaderFileName ? &GetShader(HAL::Shader::Stage::RayIntersection, *fileNames.IntersectionShaderFileName) : nullptr
        };
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