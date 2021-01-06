#include "CommandLineParser.hpp"



namespace PathFinder
{

    CommandLineParser::CommandLineParser(int argc, char** argv)
    {
        assert_format(argc > 0, "Command line arguments should contain at least one entry");

        std::filesystem::path executablePath{ argv[0] };
        mExecutableFolder = executablePath.parent_path();

        for (auto i = 1; i < argc; ++i)
        {
            ParseArgument(argv[i]);
        }
    }

    void CommandLineParser::ParseArgument(char* argv)
    {
        if (strcmp(argv, "-debug_shaders") == 0)
        {
            mBuildDebugShaders = true;
        }

        if (strcmp(argv, "-project_dir_shaders") == 0)
        {
            mUseShadersInProjectFolder = true;
        }

        if (strcmp(argv, "-debug_layer") == 0)
        {
            mDebugLayerEnabled = true;
        }

        if (strcmp(argv, "-aftermath") == 0)
        {
            mAftermathEnabled = true;
        }

        if (strcmp(argv, "-warp") == 0)
        {
            mUseWARPDevice = true;
        }

        if (strcmp(argv, "-no_memory_aliasing") == 0)
        {
            mDisableMemoryAliasing = true;
        }
    }

}
