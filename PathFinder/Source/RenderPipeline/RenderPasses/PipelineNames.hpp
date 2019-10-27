#pragma once

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    namespace ResourceNames
    {
        inline Foundation::Name GBufferRT0{ "Resource_GBuffer" };
        inline Foundation::Name BlurResult{ "Resource_BlurResult" };
        inline Foundation::Name GBufferDepthStencil{ "Resource_MainDepthStencil" };
        inline Foundation::Name DeferredLightingRT{ "Resource_DeferredLighting_RT" };
    }

    namespace PSONames
    {
        inline Foundation::Name DepthOnly{ "PSO_DepthOnly" };
        inline Foundation::Name GBuffer{ "PSO_GBuffer" };
        inline Foundation::Name DeferredLighting{ "PSO_DeferredLighting" };
        inline Foundation::Name Blur{ "PSO_Blur" };
        inline Foundation::Name BackBufferOutput{ "PSO_BackBufferOutput" };
    }  
   
    namespace RootSignatureNames
    {
        inline Foundation::Name Universal{ "Universal_Root_Sig" }; 
        inline Foundation::Name GBuffer{ "GBuffer_Root_Sig" };
        inline Foundation::Name RayTracing{ "Ray_Tracing_Root_Sig" };
    }

}
