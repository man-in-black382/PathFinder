#pragma once

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    namespace ResourceNames
    {
        inline Foundation::Name PlaygroundRenderTarget{ "Resource_PlaygroundRT" };
        inline Foundation::Name BlurResult{ "Resource_BlurResult" };
        inline Foundation::Name GBufferDepthStencil{ "Resource_MainDepthStencil" };
    }

    namespace PSONames
    {
        inline Foundation::Name DepthOnly{ "PSO_DepthOnly" };
        inline Foundation::Name GBuffer{ "PSO_GBuffer" };
        inline Foundation::Name Blur{ "PSO_Blur" }; 
    }

    namespace RootSignatureNames
    {
        inline Foundation::Name Universal{ "Universal_Root_Sig" };
    }

}
