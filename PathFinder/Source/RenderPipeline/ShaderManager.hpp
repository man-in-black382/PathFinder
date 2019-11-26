#pragma once

#include "../HardwareAbstractionLayer/Shader.hpp"
#include "../HardwareAbstractionLayer/ShaderCompiler.hpp"

#include "../IO/CommandLineParser.hpp"

#include "ShaderFileNames.hpp"

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

        HAL::GraphicsShaderBundle LoadShaders(const GraphicsShaderFileNames& fileNames);
        HAL::ComputeShaderBundle LoadShaders(const ComputeShaderFileNames& fileNames);
        HAL::RayTracingShaderBundle LoadShaders(const RayTracingShaderFileNames& fileNames);

        void SetShaderRecompilationCallback(const ShaderRecompilationCallback& callback);

        void BeginFrame();
        void EndFrame();

    private:
        struct ShaderPathAssociation
        {
            std::unordered_map<HAL::Shader::Stage, HAL::Shader*> PrimaryShaders;
            std::vector<HAL::Shader*> AffectedShaders;
        };

        HAL::Shader* GetShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);
        HAL::Shader* FindCachedShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);
        HAL::Shader* LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);

        std::filesystem::path ConstructShaderRootPath(const CommandLineParser& commandLineParser) const;
        void RecompileModifiedShaders();
        void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action) override;

        CommandLineParser mCommandLineParser;
        FW::FileWatcher mFileWatcher;
        HAL::ShaderCompiler mCompiler;
        std::filesystem::path mShaderRootPath;
        std::list<HAL::Shader> mShaders;
        std::unordered_map<std::string, ShaderPathAssociation> mShaderPathAssociations;
        std::unordered_set<std::string> mModifiedShaderFilePaths;
        ShaderRecompilationCallback mRecompilationCallback = [](const HAL::Shader* oldShader, const HAL::Shader* newShader){};
    };

}
