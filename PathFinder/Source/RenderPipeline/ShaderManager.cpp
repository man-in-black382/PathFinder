#include "ShaderManager.hpp"

#include "../Foundation/StringUtils.hpp"

namespace PathFinder
{

    ShaderManager::ShaderManager(const CommandLineParser& commandLineParser)
        : mCommandLineParser{ commandLineParser }, mShaderRootPath{ ConstructShaderRootPath(commandLineParser) }
    {
        mFileWatcher.addWatch(mShaderRootPath.string(), this, true);
    }

    HAL::GraphicsShaderBundle ShaderManager::LoadShaders(const GraphicsShaderFileNames& fileNames)
    {
        return {
            GetShader(HAL::Shader::Stage::Vertex, fileNames.VertexShaderFileName),
            GetShader(HAL::Shader::Stage::Pixel, fileNames.PixelShaderFileName),
            nullptr, nullptr,
            fileNames.GeometryShaderFileName ? GetShader(HAL::Shader::Stage::Geometry, *fileNames.GeometryShaderFileName) : nullptr
        };
    }

    HAL::ComputeShaderBundle ShaderManager::LoadShaders(const ComputeShaderFileNames& fileNames)
    {
        return { GetShader(HAL::Shader::Stage::Compute, fileNames.ComputeShaderFileName) };
    }

    HAL::RayTracingShaderBundle ShaderManager::LoadShaders(const RayTracingShaderFileNames& fileNames)
    {
        return {
            GetShader(HAL::Shader::Stage::RayGeneration, fileNames.RayGenShaderFileName),
            fileNames.ClosestHitShaderFileName ? GetShader(HAL::Shader::Stage::RayClosestHit, *fileNames.ClosestHitShaderFileName) : nullptr,
            fileNames.AnyHitShaderFileName ? GetShader(HAL::Shader::Stage::RayAnyHit, *fileNames.AnyHitShaderFileName) : nullptr,
            fileNames.MissShaderFileName ? GetShader(HAL::Shader::Stage::RayMiss, *fileNames.MissShaderFileName) : nullptr,
            fileNames.IntersectionShaderFileName ? GetShader(HAL::Shader::Stage::RayIntersection, *fileNames.IntersectionShaderFileName) : nullptr
        };
    }

    void ShaderManager::SetShaderRecompilationCallback(const ShaderRecompilationCallback& callback)
    {

    }

    void ShaderManager::BeginFrame()
    {
        // Gather shader update events. 
        // We can't just react to the first event because single
        // file modification can trigger multiple Windows events.
        // Therefore we need to gather such events across the frame
        // and discard duplicates.
        
        mFileWatcher.update();
    }

    void ShaderManager::EndFrame()
    {
        RecompileModifiedShaders();
    }

    void ShaderManager::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
    {
        switch (action)
        {
        case FW::Action::Modified:
            mModifiedShaderFilePaths.insert(dir);
            break;
        default:
            break;
        }
    }

    HAL::Shader* ShaderManager::GetShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath)
    {
        HAL::Shader* shader = FindCachedShader(pipelineStage, relativePath);
        return shader ? shader : LoadAndCacheShader(pipelineStage, relativePath);
    }

    HAL::Shader* ShaderManager::FindCachedShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath)
    {
        auto shaderAssociationIt = mShaderPathAssociations.find(relativePath.string());
        return (shaderAssociationIt == mShaderPathAssociations.end()) ? 
            nullptr : shaderAssociationIt->second.PrimaryShaders[pipelineStage];
    }

    HAL::Shader* ShaderManager::LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath)
    {
        auto fullPath = mShaderRootPath / relativePath;

        HAL::ShaderCompiler::CompilationResult compilationResult = mCompiler.Compile(fullPath, pipelineStage, mCommandLineParser.ShouldBuildDebugShaders());
        HAL::Shader* shader = &mShaders.emplace_back(std::move(compilationResult.CompiledShader));

        // Save shader with entry point as primary
        mShaderPathAssociations[relativePath.string()].PrimaryShaders[pipelineStage] = shader;

        // Save this shader as affected for all files that were used in its compilation
        for (auto& shaderFilePath : compilationResult.CompiledFileRelativePaths)
        {
            mShaderPathAssociations[shaderFilePath].AffectedShaders.push_back(shader);
        }

        return shader;
    }

    std::filesystem::path ShaderManager::ConstructShaderRootPath(const CommandLineParser& commandLineParser) const
    {
        if (commandLineParser.ShouldUseShadersFromProjectFolder())
        {
            return std::filesystem::path{ std::string(PROJECT_DIR) + "Source\\RenderPipeline\\Shaders" };
        }
        else
        {
            return commandLineParser.ExecutableFolderPath() / "Shaders";
        }
    }

    void ShaderManager::RecompileModifiedShaders()
    {
        for (auto& modifiedFilePath : mModifiedShaderFilePaths)
        {
            const ShaderPathAssociation& association = mShaderPathAssociations[modifiedFilePath];

            //for ()
        }
    }

}