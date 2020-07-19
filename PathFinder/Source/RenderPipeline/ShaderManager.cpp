#include "ShaderManager.hpp"

#include "../Foundation/StringUtils.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ShaderManager::ShaderManager(const CommandLineParser& commandLineParser, AftermathShaderDatabase* aftermathShaderDatabase)
        : mCommandLineParser{ commandLineParser }, mShaderRootPath{ ConstructShaderRootPath(commandLineParser) }, mAftermathShaderDatabase{ aftermathShaderDatabase }
    {
        mFileWatcher.addWatch(mShaderRootPath.string(), this, true);
    }

    HAL::Shader* ShaderManager::LoadShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath)
    {
        return GetShader(pipelineStage, entryPoint, relativePath);
    }

    HAL::Library* ShaderManager::LoadLibrary(const std::filesystem::path& relativePath)
    {
        return GetLibrary(relativePath);
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
        auto shaderAssociationIt = mEntryPointFilePathToCompiledObjectAssociations.find(relativePath.string());
        bool anyAssociationFound = shaderAssociationIt != mEntryPointFilePathToCompiledObjectAssociations.end();
        
        if (!anyAssociationFound)
        {
            return nullptr;
        }

        CompiledObjectsInFile& compiledObjectsInFile = shaderAssociationIt->second;
        auto bundleIt = compiledObjectsInFile.Shaders.find(entryPointName);

        if (bundleIt == compiledObjectsInFile.Shaders.end())
        {
            return nullptr;
        }

        ShaderListIterator shaderIt = bundleIt->second;
        return &(*shaderIt);
    }

    HAL::Shader* ShaderManager::LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath)
    {
        auto fullPath = mShaderRootPath / relativePath;

        HAL::ShaderCompiler::ShaderCompilationResult compilationResult = mCompiler.CompileShader(fullPath, pipelineStage, entryPoint, mCommandLineParser.ShouldBuildDebugShaders());
        
        if (!compilationResult.CompiledShader.Blob())
        {
            return nullptr;
        }

        mAftermathShaderDatabase->AddShader(compilationResult.CompiledShader);
        mShaders.emplace_back(std::move(compilationResult.CompiledShader));
        
        std::string relativePathString = relativePath.filename().string();
        ShaderListIterator shaderIt = std::prev(mShaders.end());
        
        // Associate shader with a file it was loaded from and its entry point name
        CompiledObjectsInFile& compiledObjectsInFile = mEntryPointFilePathToCompiledObjectAssociations[relativePathString];
        compiledObjectsInFile.Shaders[shaderIt->EntryPointName()] = shaderIt;

        for (auto& shaderFilePath : compilationResult.CompiledFileRelativePaths)
        {
            // Associate every file that took place in compilation with the root file that has shader's entry point
            mIncludedFilePathToEntryPointFilePathAssociations[shaderFilePath].insert(relativePathString);
        }

        return &(*shaderIt);
    }

    HAL::Library* ShaderManager::GetLibrary(const std::filesystem::path& relativePath)
    {
        HAL::Library* library = FindCachedLibrary(relativePath);

        if (!library)
        {
            library = LoadAndCacheLibrary(relativePath);
            assert_format(library, "Failed to compile library");
        }

        return library;
    }

    HAL::Library* ShaderManager::FindCachedLibrary(const std::filesystem::path& relativePath)
    {
        auto associationIt = mEntryPointFilePathToCompiledObjectAssociations.find(relativePath.string());
        bool anyAssociationFound = associationIt != mEntryPointFilePathToCompiledObjectAssociations.end();

        if (!anyAssociationFound)
        {
            return nullptr;
        }

        CompiledObjectsInFile& compiledObjectsInFile = associationIt->second;

        if (!compiledObjectsInFile.Library)
        {
            return nullptr;
        }

        LibraryListIterator libraryIt = *compiledObjectsInFile.Library;
        HAL::Library* library = &(*libraryIt);
        return library;
    }

    HAL::Library* ShaderManager::LoadAndCacheLibrary(const std::filesystem::path& relativePath)
    {
        auto fullPath = mShaderRootPath / relativePath;

        HAL::ShaderCompiler::LibraryCompilationResult compilationResult = mCompiler.CompileLibrary(fullPath, mCommandLineParser.ShouldBuildDebugShaders());

        if (!compilationResult.CompiledLibrary.Blob())
        {
            return nullptr;
        }

        mAftermathShaderDatabase->AddLibrary(compilationResult.CompiledLibrary);
        mLibraries.emplace_back(std::move(compilationResult.CompiledLibrary));

        std::string relativePathString = relativePath.filename().string();
        LibraryListIterator libraryIt = std::prev(mLibraries.end());

        CompiledObjectsInFile& compiledObjectsInFile = mEntryPointFilePathToCompiledObjectAssociations[relativePathString];
        compiledObjectsInFile.Library = libraryIt;

        for (auto& shaderFilePath : compilationResult.CompiledFileRelativePaths)
        {
            mIncludedFilePathToEntryPointFilePathAssociations[shaderFilePath].insert(relativePathString);
        }

        return &(*libraryIt);
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
        const std::unordered_set<std::string>& entryPointFileNames = mIncludedFilePathToEntryPointFilePathAssociations[modifiedFile];
        for (auto& entryPointFilePath : entryPointFileNames)
        {
            mEntryPointShaderFilesToRecompile.insert(entryPointFilePath);
        }
    }

    void ShaderManager::RecompileShader(ShaderListIterator oldShaderIt, const std::string& shaderFile)
    {
        HAL::Shader* oldShader = &(*oldShaderIt);
        HAL::Shader* newShader = LoadAndCacheShader(oldShader->PipelineStage(), oldShader->EntryPointName().ToString(), shaderFile);

        // Failed recompilation is OK
        if (!newShader)
        {
            return;
        }

        // Notify anyone interested about the old-to-new swap operation
        mShaderRecompilationEvent(oldShader, newShader);

        // Get rid of the old shader
        mShaders.erase(oldShaderIt);
    }

    void ShaderManager::RecompileLibrary(LibraryListIterator oldLibraryIt, const std::string& libraryFile)
    {
        HAL::Library* oldLibrary = &(*oldLibraryIt);
        HAL::Library* newLibrary = LoadAndCacheLibrary(libraryFile);

        if (!newLibrary)
        {
            return;
        }

        mLibraryRecompilationEvent(oldLibrary, newLibrary);
        mLibraries.erase(oldLibraryIt);
    }

    void ShaderManager::RecompileModifiedShaders()
    {
        for (auto& shaderFile : mEntryPointShaderFilesToRecompile)
        {
            const CompiledObjectsInFile& compiledObjectsInFile = mEntryPointFilePathToCompiledObjectAssociations[shaderFile];

            for (auto& [entryPointName, shaderIterator] : compiledObjectsInFile.Shaders)
            {
                RecompileShader(shaderIterator, shaderFile);
            }

            if (auto libIt = compiledObjectsInFile.Library)
            {
                RecompileLibrary(*libIt, shaderFile);
            }
        }

        mEntryPointShaderFilesToRecompile.clear();
    }

}