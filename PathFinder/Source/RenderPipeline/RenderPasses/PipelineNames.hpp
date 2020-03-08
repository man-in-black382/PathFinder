#pragma once

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    namespace ResourceNames
    {
        inline Foundation::Name JumpFloodingConesIndirection0{ "Resource_JumpFloodingConesIndirection0" };
        inline Foundation::Name JumpFloodingConesIndirection1{ "Resource_JumpFloodingConesIndirection1" };
        inline Foundation::Name JumpFloodingCones0{ "Resource_JumpFloodingConesBuffer0" };
        inline Foundation::Name JumpFloodingCones1{ "Resource_JumpFloodingConesBuffer1" };
        inline Foundation::Name UAVCounterBuffer{ "Resource_UAVCounterBuffer" };
        inline Foundation::Name GBufferRT0{ "Resource_GBuffer" };
        inline Foundation::Name BloomFullResolution{ "Resource_BloomFullResolution" };
        inline Foundation::Name BloomHalfResolution{ "Resource_BloomHalfResolution" };
        inline Foundation::Name BloomQuadResolution{ "Resource_BloomQuadResolution" };
        inline Foundation::Name GBufferDepthStencil{ "Resource_MainDepthStencil" };
        inline Foundation::Name DeferredLightingFullOutput{ "Resource_DeferredLighting_Full_Output" };
        inline Foundation::Name DeferredLightingOversaturatedOutput{ "Resource_DeferredLighting_Oversaturated_Output" };
        inline Foundation::Name ToneMappingOutput{ "Resource_ToneMapping_Output" };
    }

    namespace PSONames
    {
        inline Foundation::Name DistanceMapHelperInitialization{ "PSO_DistanceMapHelperInitialization" };
        inline Foundation::Name DistanceMapHelperCompression{ "PSO_DistanceMapHelperCompression" };
        inline Foundation::Name DistanceMapGeneration{ "PSO_DistanceMapGeneration" };
        inline Foundation::Name DepthOnly{ "PSO_DepthOnly" };
        inline Foundation::Name GBufferMeshes{ "PSO_GBufferMeshes" };
        inline Foundation::Name GBufferLights{ "PSO_GBufferLights" };
        inline Foundation::Name Shadows{ "PSO_Shadows" };
        inline Foundation::Name DeferredLighting{ "PSO_DeferredLighting" };
        inline Foundation::Name GaussianBlur{ "PSO_GaussianBlur" };
        inline Foundation::Name BloomDownscaling{ "PSO_BloomDownscaling" };
        inline Foundation::Name ToneMapping{ "PSO_ToneMapping" };
        inline Foundation::Name UI{ "PSO_UI" };
        inline Foundation::Name BackBufferOutput{ "PSO_BackBufferOutput" };
    }  
   
    namespace RootSignatureNames
    {
        inline Foundation::Name GBufferMeshes{ "GBuffer_Meshes_Root_Sig" };
        inline Foundation::Name GBufferLights{ "GBuffer_Lights_Root_Sig" };
        inline Foundation::Name DeferredLighting{ "Deferred_Lighting_Root_Sig" };
        inline Foundation::Name RayTracing{ "Ray_Tracing_Root_Sig" };
        inline Foundation::Name UI{ "UI_Root_Sig" };
        inline Foundation::Name DisplacementDistanceMapGeneration{ "Distance_Map_Generation_Root_Sig" };
    }

}
