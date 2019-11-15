#pragma once

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    namespace ResourceNames
    {
        inline Foundation::Name JumpFloodingAS0{ "Resource_JumpFloodingAS0" };
        inline Foundation::Name JumpFloodingAS1{ "Resource_JumpFloodingAS1" };

        inline Foundation::Name UAVCounterBuffer{ "Resource_UAVCounterBuffer" };
        inline Foundation::Name GBufferRT0{ "Resource_GBuffer" };
        inline Foundation::Name BlurResult{ "Resource_BlurResult" };
        inline Foundation::Name GBufferDepthStencil{ "Resource_MainDepthStencil" };
        inline Foundation::Name DeferredLightingOutput{ "Resource_DeferredLighting_Output" };
        inline Foundation::Name ToneMappingOutput{ "Resource_ToneMapping_Output" };
    }

    namespace PSONames
    {
        inline Foundation::Name DisplacementDistanceMapGeneration{ "PSO_DistanceMapGeneration" };
        inline Foundation::Name DepthOnly{ "PSO_DepthOnly" };
        inline Foundation::Name GBuffer{ "PSO_GBuffer" };
        inline Foundation::Name DeferredLighting{ "PSO_DeferredLighting" };
        inline Foundation::Name Blur{ "PSO_Blur" };
        inline Foundation::Name ToneMapping{ "PSO_ToneMapping" };
        inline Foundation::Name BackBufferOutput{ "PSO_BackBufferOutput" };
    }  
   
    namespace RootSignatureNames
    {
        inline Foundation::Name GBuffer{ "GBuffer_Root_Sig" };
        inline Foundation::Name RayTracing{ "Ray_Tracing_Root_Sig" };
        inline Foundation::Name DisplacementDistanceMapGeneration{ "Distance_Map_Generation_Root_Sig" };
    }

}
