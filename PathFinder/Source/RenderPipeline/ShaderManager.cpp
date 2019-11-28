#include "ShaderManager.hpp"

#include "../Foundation/StringUtils.hpp"
#include "../Foundation/Assert.hpp"

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
        mRecompilationCallback = callback;
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
        case FW::Action::Add:
        {
            std::filesystem::path path{ filename };
            if (path.extension().compare(".hlsl") == 0)
            {
                FindAndAddEntryPointShaderFileForRecompilation(filename);
            }
        }

        default:
            break;
        }
    }

    HAL::Shader* ShaderManager::GetShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath)
    {
        HAL::Shader* shader = FindCachedShader(pipelineStage, relativePath);

        if (!shader)
        {
            shader = LoadAndCacheShader(pipelineStage, relativePath);
            assert_format(shader, "Failed to compile shader");
        }

        return shader;
    }

    HAL::Shader* ShaderManager::FindCachedShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath)
    {
        auto shaderAssociationIt = mShaderEntryPointFilePathToShaderAssociations.find(relativePath.string());
        bool anyAssociationFound = shaderAssociationIt != mShaderEntryPointFilePathToShaderAssociations.end();
        
        if (!anyAssociationFound)
        {
            return nullptr;
        }

        ShaderListIterator shaderIt = shaderAssociationIt->second.GetShader(pipelineStage);

        if (shaderIt == mShaders.end())
        {
            return nullptr;
        }

        return &(*shaderIt);
    }

    HAL::Shader* ShaderManager::LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath)
    {
        auto fullPath = mShaderRootPath / relativePath;

        HAL::ShaderCompiler::CompilationResult compilationResult = mCompiler.Compile(fullPath, pipelineStage, mCommandLineParser.ShouldBuildDebugShaders());
        
        if (!compilationResult.CompiledShader.Blob())
        {
            return nullptr;
        }

        mShaders.emplace_back(std::move(compilationResult.CompiledShader));
        
        ShaderListIterator shaderIt = std::prev(mShaders.end());

        GetShaderBundle(relativePath.filename().string()).SetShader(shaderIt);

        for (auto& shaderFilePath : compilationResult.CompiledFileRelativePaths)
        {
            // Associate every file that took place in compilation with the root file that has shader's entry point
            mShaderAnyFilePathToEntryPointFilePathAssociations[shaderFilePath].insert(relativePath.filename().string());
        }

        return &(*shaderIt);
    }

    ShaderManager::ShaderBundle& ShaderManager::GetShaderBundle(const std::string& entryPointShaderFile)
    {
        auto it = mShaderEntryPointFilePathToShaderAssociations.find(entryPointShaderFile);
        auto shadersEndIt = mShaders.end();
        
        if (it == mShaderEntryPointFilePathToShaderAssociations.end())
        {
            ShaderBundle shaderBundle{};

            shaderBundle.VertexShader = shadersEndIt;
            shaderBundle.PixelShader = shadersEndIt;
            shaderBundle.HullShader = shadersEndIt;
            shaderBundle.DomainShader = shadersEndIt;
            shaderBundle.GeometryShader = shadersEndIt;
            shaderBundle.ComputeShader = shadersEndIt;
            shaderBundle.RayGenShader = shadersEndIt;
            shaderBundle.RayAnyHitShader = shadersEndIt;
            shaderBundle.RayClosestHitShader = shadersEndIt;
            shaderBundle.RayMissShader = shadersEndIt;
            shaderBundle.RayIntersectionShader = shadersEndIt;

            auto [iter, success] = mShaderEntryPointFilePathToShaderAssociations.emplace(entryPointShaderFile, shaderBundle);
            return iter->second;
        }
        else 
        {
            return it->second;
        }
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

    void ShaderManager::FindAndAddEntryPointShaderFileForRecompilation(const std::string& modifiedFile)
    {
        const std::unordered_set<std::string>& entryPointFileNames = mShaderAnyFilePathToEntryPointFilePathAssociations[modifiedFile];
        for (auto& entryPointFilePath : entryPointFileNames)
        {
            mEntryPointShaderFilesToRecompile.insert(entryPointFilePath);
        }
    }

    void ShaderManager::RecompileShader(ShaderListIterator oldShaderIt, const std::string& shaderFile)
    {
        if (oldShaderIt == mShaders.end())
        {
            return;
        }

        HAL::Shader* oldShader = &(*oldShaderIt);
        HAL::Shader* newShader = LoadAndCacheShader(oldShader->PipelineStage(), shaderFile);

        // Failed recompilation is OK
        if (!newShader)
        {
            return;
        }

        // Notify anyone interested about the old-to-new swap operation
        mRecompilationCallback(oldShader, newShader);

        // Get rid of the old shader
        mShaders.erase(oldShaderIt);
    }

    void ShaderManager::RecompileModifiedShaders()
    {
        for (auto& shaderFile : mEntryPointShaderFilesToRecompile)
        {
            const ShaderBundle& shaderBundle = mShaderEntryPointFilePathToShaderAssociations[shaderFile];

            RecompileShader(shaderBundle.VertexShader, shaderFile);
            RecompileShader(shaderBundle.PixelShader, shaderFile);
            RecompileShader(shaderBundle.HullShader, shaderFile);
            RecompileShader(shaderBundle.DomainShader, shaderFile);
            RecompileShader(shaderBundle.GeometryShader, shaderFile);
            RecompileShader(shaderBundle.ComputeShader, shaderFile);
            RecompileShader(shaderBundle.RayGenShader, shaderFile);
            RecompileShader(shaderBundle.RayAnyHitShader, shaderFile);
            RecompileShader(shaderBundle.RayClosestHitShader, shaderFile);
            RecompileShader(shaderBundle.RayMissShader, shaderFile);
            RecompileShader(shaderBundle.RayIntersectionShader, shaderFile);
        }

        mEntryPointShaderFilesToRecompile.clear();
    }

    void ShaderManager::ShaderBundle::SetShader(ShaderListIterator it)
    {
        switch (it->PipelineStage())
        {
        case HAL::Shader::Stage::Vertex: VertexShader = it; break;
        case HAL::Shader::Stage::Hull: HullShader = it; break;
        case HAL::Shader::Stage::Domain: DomainShader = it; break;
        case HAL::Shader::Stage::Geometry: GeometryShader = it; break;
        case HAL::Shader::Stage::Pixel: PixelShader = it; break;
        case HAL::Shader::Stage::Compute: ComputeShader = it; break;
        case HAL::Shader::Stage::RayGeneration: RayGenShader = it; break;
        case HAL::Shader::Stage::RayClosestHit: RayClosestHitShader = it; break;
        case HAL::Shader::Stage::RayAnyHit: RayAnyHitShader = it; break;
        case HAL::Shader::Stage::RayMiss: RayMissShader = it; break;
        case HAL::Shader::Stage::RayIntersection: RayIntersectionShader = it; break;
        default: break;
        }
    }

    ShaderManager::ShaderListIterator ShaderManager::ShaderBundle::GetShader(HAL::Shader::Stage stage) const
    {
        switch (stage)
        {
        case HAL::Shader::Stage::Vertex: return VertexShader;
        case HAL::Shader::Stage::Hull: return HullShader;
        case HAL::Shader::Stage::Domain: return DomainShader;
        case HAL::Shader::Stage::Geometry: return GeometryShader;
        case HAL::Shader::Stage::Pixel: return PixelShader;
        case HAL::Shader::Stage::Compute: return ComputeShader;
        case HAL::Shader::Stage::RayGeneration: return RayGenShader;
        case HAL::Shader::Stage::RayClosestHit: return RayClosestHitShader;
        case HAL::Shader::Stage::RayAnyHit: return RayAnyHitShader;
        case HAL::Shader::Stage::RayMiss: return RayMissShader;
        case HAL::Shader::Stage::RayIntersection: return RayIntersectionShader;
        default: return ShaderListIterator{};
        }
    }

}