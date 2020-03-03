#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"
#include "../Memory/GPUResourceProducer.hpp"

#include "ShaderManager.hpp"
#include "RenderSurfaceDescription.hpp"
#include "PipelineStateProxy.hpp"
#include "RootSignatureProxy.hpp"
#include "ShaderFileNames.hpp"

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
            PipelineStateVariant(const HAL::RayTracingPipelineState* rtState) : RayTracingPSO{ rtState } {}

            const HAL::GraphicsPipelineState* GraphicPSO = nullptr;
            const HAL::ComputePipelineState* ComputePSO = nullptr;
            const HAL::RayTracingPipelineState* RayTracingPSO = nullptr;
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
        };

        // Store graphic and compute states directly, but store ray tracing one in a wrapper because we need to manage and associate additional memory with it
        using PipelineStateVariantInternal = std::variant<HAL::GraphicsPipelineState, HAL::ComputePipelineState, RayTracingStateWrapper>;

        const HAL::RootSignature* GetNamedRootSignatureOrDefault(std::optional<RootSignatureName> name) const;
        const HAL::RootSignature* GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const;

        void AssociateStateWithShaders(PipelineStateVariantInternal* state, const HAL::GraphicsShaderBundle& shaders);
        void AssociateStateWithShaders(PipelineStateVariantInternal* state, const HAL::ComputeShaderBundle& shaders);
        void AssociateStateWithShaders(PipelineStateVariantInternal* state, const HAL::RayTracingShaderBundle& shaders);

        void ConfigureDefaultStates();
        void BuildBaseRootSignature(); 
        void CompileRayTracingState(RayTracingStateWrapper& stateWrapper, Foundation::Name psoName);

        ShaderManager* mShaderManager; 
        Memory::GPUResourceProducer* mResourceProducer;
        RenderSurfaceDescription mDefaultRenderSurfaceDesc;
        
        HAL::Device* mDevice;
        HAL::RootSignature mBaseRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;

        std::unordered_map<PSOName, PipelineStateVariantInternal> mPipelineStates;
        std::unordered_map<RootSignatureName, HAL::RootSignature> mRootSignatures;
        std::unordered_map<const HAL::Shader*, std::unordered_set<PipelineStateVariantInternal*>> mShaderToPSOAssociations;
        std::unordered_set<PipelineStateVariantInternal*> mStatesToRecompile;
    };

}
