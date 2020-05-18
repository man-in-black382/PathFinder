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

        inline Foundation::Name GBufferAlbedoMetalness{ "Resource_GBuffer_Albedo_Metalness" };
        inline Foundation::Name GBufferNormalRoughness{ "Resource_GBuffer_Normal_Roughness" };
        inline Foundation::Name GBufferMotionVector{ "Resource_GBuffer_Motion_Vector" };
        inline Foundation::Name GBufferTypeAndMaterialIndex{ "Resource_GBuffer_Type_And_Material_Index" };
        inline Foundation::Name GBufferDepthStencil{ "Resource_GBuffer_Depth_Stencil" };
        inline Foundation::Name GBufferViewDepth{ "Resource_GBuffer_View_Depth" };

        inline Foundation::Name DeferredLightingFullOutput{ "Resource_DeferredLighting_Full_Output" };
        inline Foundation::Name DeferredLightingOverexposedOutput{ "Resource_DeferredLighting_Overexposed_Output" };
        inline Foundation::Name ShadingAnalyticOutput{ "Resource_Shading_Analytic_Output" };
        //inline Foundation::Name ShadingStochasticOutput{ "Resource_Shading_Stochastic_Output" };
        //inline Foundation::Name ShadingVisibilityOutput{ "Resource_Shaading_Visibility__Output" };
        inline Foundation::Name ShadingStochasticUnshadowedOutput{ "Resource_Shading_Stochastic_Unshadowed_Output" };
        inline Foundation::Name ShadingStochasticShadowedOutput{ "Resource_Shading_Stochastic_Shadowed_Output" };
        inline Foundation::Name DenoiserReprojectedFramesCount{ "Resource_Denoiser_Reprojected_Frames_Count" };
        //inline Foundation::Name DenoisingStochasticShadowedIntermediateTarget{ "Resource_Denoising_Stochastic_Shadowed_Intermediate_Target" };
        //inline Foundation::Name DenoisingStochasticUnsadowedIntermediateTarget{ "Resource_Denoising_Stochastic_Unshadowed_Intermediate_Target" };
        //inline Foundation::Name ShadowDenoisingOutput{ "Resource_Shadow_Denoising_Output" };
        //inline Foundation::Name ShadowNoiseEstimationOutput{ "Resource_Shadow_Noise_Estimation_Output" };
        //inline Foundation::Name ShadowNoiseEstimationDenoisingOutput{ "Resource_Shadow_Noise_Estimation_Denoising_Output" };
        inline Foundation::Name BloomBlurIntermediate{ "Resource_Bloom_Blur_Intermediate" };
        inline Foundation::Name BloomBlurOutput{ "Resource_Bloom_Blur_Output" };
        inline Foundation::Name BloomCompositionOutput{ "Resource_Bloom_Composition_Output" };
        inline Foundation::Name ToneMappingOutput{ "Resource_ToneMapping_Output" };
    }

    namespace PSONames
    {
        inline Foundation::Name DistanceMapHelperInitialization{ "PSO_DistanceMapHelperInitialization" };
        inline Foundation::Name DistanceMapHelperCompression{ "PSO_DistanceMapHelperCompression" };
        inline Foundation::Name DistanceMapGeneration{ "PSO_DistanceMapGeneration" };
        inline Foundation::Name AveragindDownsampling{ "PSO_AveragingDownsampling" };
        inline Foundation::Name DepthOnly{ "PSO_DepthOnly" };
        inline Foundation::Name GBufferMeshes{ "PSO_GBufferMeshes" };
        inline Foundation::Name GBufferLights{ "PSO_GBufferLights" };
        inline Foundation::Name Shading{ "PSO_Shading" };
        inline Foundation::Name DeferredLighting{ "PSO_DeferredLighting" };
        inline Foundation::Name GaussianBlur{ "PSO_GaussianBlur" };
        inline Foundation::Name BloomDownscaling{ "PSO_BloomDownscaling" };
        inline Foundation::Name BloomBlur{ "PSO_BloomBlur" };
        inline Foundation::Name BloomComposition{ "PSO_BloomComposition" };
        inline Foundation::Name ToneMapping{ "PSO_ToneMapping" };
        inline Foundation::Name ShadowNoiseEstimation{ "PSO_ShadowNoiseEstimation" };
        inline Foundation::Name ShadowNoiseEstimationDenoising{ "PSO_ShadowNoiseEstimationDenoising" };
        inline Foundation::Name ShadowDenoising{ "PSO_ShadowDenoising" };
        inline Foundation::Name DenoiserReprojection{ "PSO_DenoiserReprojection" };
        inline Foundation::Name SpecularDenoiser{ "PSO_SpecularDenoiser" };
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
