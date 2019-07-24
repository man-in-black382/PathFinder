#pragma once

#include "../HardwareAbstractionLayer/Shader.hpp"

namespace PathFinder
{

    class IShaderManager
    {
    public:
        virtual ShaderBundle LoadShaders(const std::string& vsFileName, const std::string& psFileName) = 0;
        virtual ShaderBundle LoadShaders(const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName) = 0;
        virtual ShaderBundle LoadShaders(const std::string& csFileName) = 0;
    };

}
