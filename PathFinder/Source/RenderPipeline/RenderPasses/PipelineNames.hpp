#pragma once

#include "../Foundation/Name.hpp"

#include <array>

namespace PathFinder
{

    template <size_t Count>
    using NameArray = std::array<Foundation::Name, Count>;

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
        inline NameArray<2> GBufferViewDepth{ "Resource_GBuffer_View_Depth[0]", "Resource_GBuffer_View_Depth[1]" };

        inline Foundation::Name ShadingAnalyticOutput{ "Resource_Shading_Analytic_Output" };

        inline Foundation::Name StochasticUnshadowedShadingOutput{ "Resource_Shading_Stochastic_Unshadowed_Output" };
        inline Foundation::Name StochasticShadowedShadingOutput{ "Resource_Shading_Stochastic_Shadowed_Output" };
        inline Foundation::Name StochasticUnshadowedShadingPreBlurred{ "Resource_Shading_Stochastic_Unshadowed_Pre_Blurred" };
        inline Foundation::Name StochasticShadowedShadingPreBlurred{ "Resource_Shading_Stochastic_Shadowed_Pre_Blurred" };
        inline Foundation::Name StochasticUnshadowedShadingFixed{ "Resource_Shading_Stochastic_Unshadowed_Fixed" };
        inline Foundation::Name StochasticShadowedShadingFixed{ "Resource_Shading_Stochastic_Shadowed_Fixed" };
        inline Foundation::Name StochasticUnshadowedShadingReprojected{ "Resource_Shading_Stochastic_Unshadowed_Reprojected_History" };
        inline Foundation::Name StochasticShadowedShadingReprojected{ "Resource_Shading_Stochastic_Shadowed_Reprojected_History" };
        inline NameArray<2> StochasticUnshadowedShadingDenoised{ "Resource_Shading_Stochastic_Unshadowed_Denoised[0]", "Resource_Shading_Stochastic_Unshadowed_Denoised[1]" };
        inline NameArray<2> StochasticShadowedShadingDenoised{ "Resource_Shading_Stochastic_Shadowed_Denoised[0]", "Resource_Shading_Stochastic_Shadowed_Denoised[1]" };
        inline Foundation::Name StochasticUnshadowedShadingDenoisedStabilized{ "Resource_Shading_Stochastic_Unshadowed_Denoised_Stabilized" };
        inline Foundation::Name StochasticShadowedShadingDenoisedStabilized{ "Resource_Shading_Stochastic_Shadowed_Post_Blurred" };
        inline Foundation::Name StochasticUnshadowedShadingPostBlurred{ "Resource_Shading_Stochastic_Unshadowed_Post_Blurred" };
        inline Foundation::Name StochasticShadowedShadingPostBlurred{ "Resource_Shading_Stochastic_Shadowed_Post_Blurred" };
        inline Foundation::Name CombinedShading{ "Resource_Shading_Combined" };
        inline Foundation::Name CombinedShadingOverexposed{ "Resource_Shading_Combined_Overexposed" };
        inline Foundation::Name DenoiserPrimaryGradientInputs{ "Denoiser_Primary_Gradient_Inputs" };
        inline Foundation::Name DenoiserPrimaryGradient{ "Denoiser_Primary_Gradient" };
        inline Foundation::Name DenoiserSecondaryGradient{ "Denoiser_Secondary_Gradient" };
        inline Foundation::Name DenoiserPrimaryGradientFiltered{ "Denoiser_Primary_Gradient_Filtered" };
        inline Foundation::Name DenoiserPrimaryGradientFilteredIntermediate{ "Denoiser_Primary_Gradient_Filtered_Intermediate" };
        inline NameArray<2> RngSeeds{ "Resource_Stochastic_Rng_Seeds[0]", "Resource_Stochastic_Rng_Seeds[1]" };
        inline Foundation::Name RngSeedsCorrelated{ "Resource_Stochastic_Rng_Seeds_Correlated" };

        inline NameArray<2> DenoiserGradientSamplePositions{ "Resource_Denoiser_Gradient_Sample_Positions[0]", "Resource_Denoiser_Gradient_Sample_Positions[1]" };
        inline Foundation::Name DenoisedPreBlurIntermediate{ "Resource_Denoised_Pre_Blur_Intermediate" };
        inline NameArray<2> DenoiserReprojectedFramesCount{ "Resource_Denoiser_Reprojected_Frames_Count[0]", "Resource_Denoiser_Reprojected_Frames_Count[1]" };
        inline Foundation::Name DenoisedCombinedDirectShading{ "Resource_Denoised_Combined_Direct_Shading" };
        inline Foundation::Name BloomBlurIntermediate{ "Resource_Bloom_Blur_Intermediate" };
        inline Foundation::Name BloomBlurOutput{ "Resource_Bloom_Blur_Output" };
        inline Foundation::Name BloomCompositionOutput{ "Resource_Bloom_Composition_Output" };
        inline Foundation::Name ToneMappingOutput{ "Resource_ToneMapping_Output" };
        inline Foundation::Name UIOutput{ "Resource_UI_Output" };

        inline Foundation::Name SMAADetectedEdges{ "Resource_SMAA_Detected_Edges" };
        inline Foundation::Name SMAABlendingWeights{ "Resource_SMAA_Blending_Weights" };
        inline Foundation::Name SMAAAntialiased{ "Resource_SMAA_Antialiased_Image" };
    }

    namespace PSONames
    {
        inline Foundation::Name DistanceMapHelperInitialization{ "PSO_DistanceMapHelperInitialization" };
        inline Foundation::Name DistanceMapHelperCompression{ "PSO_DistanceMapHelperCompression" };
        inline Foundation::Name DistanceMapGeneration{ "PSO_DistanceMapGeneration" };
        inline Foundation::Name Downsampling{ "PSO_AveragingDownsampling" };
        inline Foundation::Name DepthOnly{ "PSO_DepthOnly" };
        inline Foundation::Name GBufferMeshes{ "PSO_GBufferMeshes" };
        inline Foundation::Name GBufferLights{ "PSO_GBufferLights" };
        inline Foundation::Name Shading{ "PSO_Shading" };
        inline Foundation::Name DeferredLighting{ "PSO_DeferredLighting" };
        inline Foundation::Name SeparableBlur{ "PSO_SeparableBlur" };
        inline Foundation::Name BloomBlur{ "PSO_BloomBlur" };
        inline Foundation::Name BloomComposition{ "PSO_BloomComposition" };
        inline Foundation::Name ToneMapping{ "PSO_ToneMapping" };
        inline Foundation::Name RngSeedGeneration{ "PSO_RngSeedGeneration" };
        inline Foundation::Name DenoiserForwardProjection{ "PSO_DenoiserForwardProjection" };
        inline Foundation::Name DenoiserReprojection{ "PSO_DenoiserReprojection" };
        inline Foundation::Name DenoiserHistoryFix{ "PSO_DenoiserHistoryFix" };
        inline Foundation::Name DenoiserPostStabilization{ "PSO_DenoiserPostStabilization" };
        inline Foundation::Name DenoiserPostBlur{ "PSO_DenoiserPostBlur" };
        inline Foundation::Name DenoiserGradientConstruction{ "PSO_DenoiserGradientConstruction" };
        inline Foundation::Name DenoiserGradientFiltering{ "PSO_DenoiserGradientFiltering" };
        inline Foundation::Name SpecularDenoiser{ "PSO_SpecularDenoiser" };
        inline Foundation::Name SMAAEdgeDetection{ "PSO_SMAAEdgeDetection" };
        inline Foundation::Name SMAABlendingWeightCalculation{ "PSO_SMAABlendingWeightCalculation" };
        inline Foundation::Name SMAANeighborhoodBlending{ "PSO_SMAANeighborhoodBlending" };
        inline Foundation::Name UI{ "PSO_UI" };
        inline Foundation::Name BackBufferOutput{ "PSO_BackBufferOutput" };
        inline Foundation::Name UAVClear{ "PSO_UAVClear" };
        inline Foundation::Name BoxBlur{ "PSO_BoxBlur" };
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

    namespace SamplerNames
    {
        inline Foundation::Name AnisotropicClamp{ "Sampler_Anisotropic_Clamp" };
        inline Foundation::Name LinearClamp{ "Sampler_Linear_Clamp" };
        inline Foundation::Name PointClamp{ "Sampler_Point_Clamp" };
        inline Foundation::Name Minimim{ "Sampler_Minimum" };
        inline Foundation::Name Maximum{ "Sampler_Maximum" };
    }

}
