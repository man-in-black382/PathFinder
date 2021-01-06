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
        bool mAftermathEnabled = false;
        bool mUseWARPDevice = false;
        bool mDisableMemoryAliasing = false;

    public:
        inline auto ShouldEnableDebugLayer() const { return mDebugLayerEnabled; }
        inline auto ShouldBuildDebugShaders() const { return mBuildDebugShaders; }
        inline auto ShouldUseShadersFromProjectFolder() const { return mUseShadersInProjectFolder; }
        inline auto ShouldEnableAftermath() const { return mAftermathEnabled; }
        inline auto ShouldUseWARPDevice() const { return mUseWARPDevice; }
        inline auto DisableMemoryAliasing() const { return mDisableMemoryAliasing; }
        inline const auto& ExecutableFolderPath() const { return mExecutableFolder; }
    };

}
