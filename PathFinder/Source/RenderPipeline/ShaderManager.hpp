#pragma once

#include "../HardwareAbstractionLayer/Shader.hpp"
#include "../HardwareAbstractionLayer/ShaderCompiler.hpp"

#include "../IO/CommandLineParser.hpp"

#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <functional>
#include <filewatch/FileWatcher.h>

namespace PathFinder
{

    class ShaderManager : private FW::FileWatchListener
    {
    public:
        using ShaderRecompilationCallback = std::function<void(const HAL::Shader*, const HAL::Shader*)>;

        ShaderManager(const CommandLineParser& commandLineParser);

        HAL::Shader* LoadShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath);

        void SetShaderRecompilationCallback(const ShaderRecompilationCallback& callback);

        void BeginFrame();
        void EndFrame();

    private:
        using EntryPointName = Foundation::Name;
        using ShaderListIterator = std::list<HAL::Shader>::iterator;
        using ShaderBundle = std::unordered_map<EntryPointName, ShaderListIterator>;

        HAL::Shader* GetShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath);
        HAL::Shader* FindCachedShader(Foundation::Name entryPointName, const std::filesystem::path& relativePath);
        HAL::Shader* LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::string& entryPoint, const std::filesystem::path& relativePath);
        ShaderBundle& GetShaderBundle(const std::string& entryPointShaderFile);

        std::filesystem::path ConstructShaderRootPath(const CommandLineParser& commandLineParser) const;
        void FindAndAddEntryPointShaderFileForRecompilation(const std::string& modifiedFile);
        void RecompileShader(ShaderListIterator oldShaderIt, const std::string& shaderFile);
        void RecompileModifiedShaders();
        void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action) override;

        CommandLineParser mCommandLineParser;
        FW::FileWatcher mFileWatcher;
        HAL::ShaderCompiler mCompiler;
        std::filesystem::path mShaderRootPath;
        std::list<HAL::Shader> mShaders;
        std::unordered_set<std::string> mEntryPointShaderFilesToRecompile;
        std::unordered_map<std::string, ShaderBundle> mShaderEntryPointFilePathToShaderAssociations;
        std::unordered_map<std::string, std::unordered_set<std::string>> mShaderAnyFilePathToEntryPointFilePathAssociations;
        ShaderRecompilationCallback mRecompilationCallback = [](const HAL::Shader* oldShader, const HAL::Shader* newShader){};
    };

}
