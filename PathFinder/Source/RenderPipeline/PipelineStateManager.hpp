#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"
#include "../Memory/GPUResourceProducer.hpp"

#include "ShaderManager.hpp"
#include "RenderSurfaceDescription.hpp"
#include "PipelineStateProxy.hpp"
#include "RootSignatureProxy.hpp"

#include <unordered_map>

namespace PathFinder
{
    using PSOName = Foundation::Name;
    using RootSignatureName = Foundation::Name;

    class PipelineStateManager
    {
    public:
        using RootSignatureConfigurator = std::function<void(RootSignatureProxy&)>;
        using GraphicsStateConfigurator = std::function<void(GraphicsStateProxy&)>;
        using ComputeStateConfigurator = std::function<void(ComputeStateProxy&)>;
        using RayTracingStateConfigurator = std::function<void(RayTracingStateProxy&)>;

        struct PipelineStateVariant
        {
            PipelineStateVariant(const HAL::GraphicsPipelineState* graphicState) : GraphicPSO{ graphicState } {}
            PipelineStateVariant(const HAL::ComputePipelineState* computeState) : ComputePSO{ computeState } {}
            PipelineStateVariant(const HAL::RayTracingPipelineState* rtState, const HAL::RayDispatchInfo* rayDispatchInfo) 
                : RayTracingPSO{ rtState }, BaseRayDispatchInfo{ rayDispatchInfo } {}

            const HAL::GraphicsPipelineState* GraphicPSO = nullptr;
            const HAL::ComputePipelineState* ComputePSO = nullptr;
            const HAL::RayTracingPipelineState* RayTracingPSO = nullptr;
            const HAL::RayDispatchInfo* BaseRayDispatchInfo = nullptr;
        };

        PipelineStateManager(
            HAL::Device* device,
            ShaderManager* shaderManager,
            Memory::GPUResourceProducer* resourceProducer, 
            const RenderSurfaceDescription& defaultRenderSurface
        );

        void CreateRootSignature(RootSignatureName name, const RootSignatureConfigurator& configurator);
        void CreateGraphicsState(PSOName name, const GraphicsStateConfigurator& configurator);
        void CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator);
        void CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator);

        std::optional<PipelineStateVariant> GetPipelineState(PSOName name) const;
        const HAL::RootSignature* GetRootSignature(RootSignatureName name) const;
        const HAL::RootSignature& BaseRootSignature() const;

        void CompileSignaturesAndStates();
        void RecompileModifiedStates();
        bool HasModifiedStates() const;

    private:
        struct RayTracingStateWrapper
        {
            HAL::RayTracingPipelineState State;
            Memory::GPUResourceProducer::BufferPtr ShaderTableBuffer;

            // Dispatch info containing shader table memory layout
            // but not actual dispatch dimensions. Clone this
            // dispatch info and set dimensions for the copy.
            std::optional<HAL::RayDispatchInfo> BaseRayDispatchInfo;
        };

        // Store graphic and compute states directly, but store ray tracing one in a wrapper because we need to manage and associate additional memory with it
        using PipelineStateVariantInternal = std::variant<HAL::GraphicsPipelineState, HAL::ComputePipelineState, RayTracingStateWrapper>;

        const HAL::RootSignature* GetNamedRootSignatureOrDefault(std::optional<RootSignatureName> name) const;
        const HAL::RootSignature* GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const;

        void AssociateStateWithShader(PipelineStateVariantInternal* state, const HAL::Shader* shader);
        void AssociateStateWithLibrary(PipelineStateVariantInternal* state, const HAL::Library* library);

        void ConfigureDefaultStates();
        void BuildBaseRootSignature(); 
        void CompileRayTracingState(RayTracingStateWrapper& stateWrapper, Foundation::Name psoName);

        void RecompileStatesWithNewShader(const HAL::Shader* oldShader, const HAL::Shader* newShader);
        void RecompileStatesWithNewLibrary(const HAL::Library* oldLibrary, const HAL::Library* newLibrary);

        ShaderManager* mShaderManager; 
        Memory::GPUResourceProducer* mResourceProducer;
        RenderSurfaceDescription mDefaultRenderSurfaceDesc;
        
        HAL::Device* mDevice;
        HAL::RootSignature mBaseRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;

        std::unordered_map<PSOName, PipelineStateVariantInternal> mPipelineStates;
        std::unordered_map<RootSignatureName, HAL::RootSignature> mRootSignatures;
        std::unordered_map<const HAL::Shader*, std::unordered_set<PipelineStateVariantInternal*>> mShaderToPSOAssociations;
        std::unordered_map<const HAL::Library*, std::unordered_set<PipelineStateVariantInternal*>> mLibraryToPSOAssociations;
        std::unordered_set<PipelineStateVariantInternal*> mStatesToRecompile;

        std::string mDefaultVertexEntryPointName = "VSMain";
        std::string mDefaultPixelEntryPointName = "PSMain";
        std::string mDefaultGeometryEntryPointName = "GSMain";
        std::string mDefaultComputeEntryPointName = "CSMain";
        std::string mDefaultRayGenerationEntryPointName = "RayGeneration";
        std::string mDefaultRayMissEntryPointName = "RayMiss";
        std::string mDefaultRayAnyHitEntryPointName = "RayAnyHit";
        std::string mDefaultRayClosestHitEntryPointName = "RayClosestHit";
        std::string mDefaultRayIntersectionEntryPointName = "RayIntersection";
    };

}
