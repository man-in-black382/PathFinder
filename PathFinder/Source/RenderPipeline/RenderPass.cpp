#include "RenderPass.hpp"

namespace PathFinder
{

    RenderPass::RenderPass(Foundation::Name name, const std::string& vsFileName, const std::string& psFileName)
        : mName{ name },
        mVertexShaderFileName{ vsFileName },
        mPixelShaderFileName{ psFileName } {}

    RenderPass::RenderPass(Foundation::Name name, const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName)
        : mName(name),
        mVertexShaderFileName{ vsFileName },
        mGeometryShaderFileName{ gsFileName },
        mPixelShaderFileName{ psFileName } {}

    RenderPass::RenderPass(Foundation::Name name, const std::string& csFileName)
        : mName(name),
        mComputeShaderFileName{ csFileName } {}

}
