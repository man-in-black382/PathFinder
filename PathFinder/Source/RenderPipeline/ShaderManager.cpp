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

    HAL::Shader* ShaderManager::LoadShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath)
    {
        return GetShader(pipelineStage, entryPoint, relativePath);
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

    HAL::Shader* ShaderManager::GetShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath)
    {
        HAL::Shader* shader = FindCachedShader(entryPoint, relativePath);

        if (!shader)
        {
            shader = LoadAndCacheShader(pipelineStage, entryPoint, relativePath);
            assert_format(shader, "Failed to compile shader");
        }

        return shader;
    }

    HAL::Shader* ShaderManager::FindCachedShader(Foundation::Name entryPointName, const std::filesystem::path& relativePath)
    {
        auto shaderAssociationIt = mShaderEntryPointFilePathToShaderAssociations.find(relativePath.string());
        bool anyAssociationFound = shaderAssociationIt != mShaderEntryPointFilePathToShaderAssociations.end();
        
        if (!anyAssociationFound)
        {
            return nullptr;
        }

        ShaderBundle& shadersForFile = shaderAssociationIt->second;
        auto bundleIt = shadersForFile.find(entryPointName);

        if (bundleIt == shadersForFile.end())
        {
            return nullptr;
        }

        ShaderListIterator shaderIt = bundleIt->second;
        return &(*shaderIt);
    }

    HAL::Shader* ShaderManager::LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath)
    {
        auto fullPath = mShaderRootPath / relativePath;

        HAL::ShaderCompiler::CompilationResult compilationResult = mCompiler.Compile(fullPath, pipelineStage, entryPoint, mCommandLineParser.ShouldBuildDebugShaders());
        
        if (!compilationResult.CompiledShader.Blob())
        {
            return nullptr;
        }

        mShaders.emplace_back(std::move(compilationResult.CompiledShader));
        
        std::string relativePathString = relativePath.filename().string();
        ShaderListIterator shaderIt = std::prev(mShaders.end());
        
        // Associate shader with a file it was loaded from and its entry point name
        ShaderBundle& shadersForFile = mShaderEntryPointFilePathToShaderAssociations[relativePathString];
        shadersForFile.emplace(shaderIt->EntryPointName(), shaderIt);

        for (auto& shaderFilePath : compilationResult.CompiledFileRelativePaths)
        {
            // Associate every file that took place in compilation with the root file that has shader's entry point
            mShaderAnyFilePathToEntryPointFilePathAssociations[shaderFilePath].insert(relativePathString);
        }

        return &(*shaderIt);
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
        HAL::Shader* newShader = LoadAndCacheShader(oldShader->PipelineStage(), oldShader->EntryPointName().ToString(), shaderFile);

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

            for (auto& [entryPointName, shaderIterator] : shaderBundle)
            {
                RecompileShader(shaderIterator, shaderFile);
            }
        }

        mEntryPointShaderFilesToRecompile.clear();
    }

}