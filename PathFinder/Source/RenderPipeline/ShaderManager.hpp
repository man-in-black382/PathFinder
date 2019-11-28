#pragma once

#include "../HardwareAbstractionLayer/Shader.hpp"
#include "../HardwareAbstractionLayer/ShaderCompiler.hpp"

#include "../IO/CommandLineParser.hpp"

#include "ShaderFileNames.hpp"

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

        HAL::GraphicsShaderBundle LoadShaders(const GraphicsShaderFileNames& fileNames);
        HAL::ComputeShaderBundle LoadShaders(const ComputeShaderFileNames& fileNames);
        HAL::RayTracingShaderBundle LoadShaders(const RayTracingShaderFileNames& fileNames);

        void SetShaderRecompilationCallback(const ShaderRecompilationCallback& callback);

        void BeginFrame();
        void EndFrame();

    private:
        using ShaderListIterator = std::list<HAL::Shader>::iterator;

        struct ShaderBundle
        {
            ShaderListIterator ComputeShader;
            ShaderListIterator VertexShader;
            ShaderListIterator PixelShader;
            ShaderListIterator HullShader;
            ShaderListIterator DomainShader;
            ShaderListIterator GeometryShader;
            ShaderListIterator RayGenShader;
            ShaderListIterator RayClosestHitShader;
            ShaderListIterator RayAnyHitShader;
            ShaderListIterator RayMissShader;
            ShaderListIterator RayIntersectionShader;

            void SetShader(ShaderListIterator it);
            ShaderListIterator GetShader(HAL::Shader::Stage stage) const;
        };

        HAL::Shader* GetShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);
        HAL::Shader* FindCachedShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);
        HAL::Shader* LoadAndCacheShader(HAL::Shader::Stage pipelineStage, const std::filesystem::path& relativePath);
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
