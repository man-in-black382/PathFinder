#pragma once

#include <filesystem>

namespace PathFinder 
{

    class CommandLineParser
    {
    public:
        CommandLineParser(int argc, char** argv);

    private:
        void ParseArgument(char* argv);

        std::filesystem::path mExecutableFolder;
        bool mBuildDebugShaders = false;
        bool mUseShadersInProjectFolder = false;
        bool mDebugLayerEnabled = false;

    public:
        inline auto ShouldEnableDebugLayer() const { return mDebugLayerEnabled; }
        inline auto ShouldBuildDebugShaders() const { return mBuildDebugShaders; }
        inline auto ShouldUseShadersFromProjectFolder() const { return mUseShadersInProjectFolder; }
        inline const auto& ExecutableFolderPath() const { return mExecutableFolder; }
    };

}
