#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"

#include "ShaderManager.hpp"
#include "RenderSurfaceDescription.hpp"
#include "PipelineStateProxy.hpp"
#include "ShaderFileNames.hpp"

#include <unordered_map>

namespace PathFinder
{
    using PSOName = Foundation::Name;
    using RootSignatureName = Foundation::Name;

    class PipelineStateManager
    {
    public:
        using GraphicsStateConfigurator = std::function<void(GraphicsStateProxy&)>;
        using ComputeStateConfigurator = std::function<void(ComputeStateProxy&)>;
        using RayTracingStateConfigurator = std::function<void(RayTracingStateProxy&)>;

        using PipelineStateVariant = std::variant<HAL::GraphicsPipelineState, HAL::ComputePipelineState, HAL::RayTracingPipelineState>;
        using StateConfiguratorVariant = std::variant<GraphicsStateConfigurator, ComputeStateConfigurator, RayTracingStateConfigurator>;

        PipelineStateManager(HAL::Device* device, ShaderManager* shaderManager, const RenderSurfaceDescription& defaultRenderSurface);

        void StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature);
        void CreateGraphicsState(PSOName name, const GraphicsStateConfigurator& configurator);
        void CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator);
        void CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator);

        const PipelineStateVariant* GetPipelineState(PSOName name) const;
        const HAL::RootSignature* GetRootSignature(RootSignatureName name) const;
        const HAL::RootSignature& BaseRootSignature() const;

        void CompileStates();
        void RecompileModifiedStates();
        bool HasModifiedStates() const;

    private:
        const HAL::RootSignature* GetNamedRootSignatureOrDefault(std::optional<RootSignatureName> name) const;
        const HAL::RootSignature* GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const;

        void AssociateStateWithShaders(PipelineStateVariant* state, const HAL::GraphicsShaderBundle& shaders);
        void AssociateStateWithShaders(PipelineStateVariant* state, const HAL::ComputeShaderBundle& shaders);
        void AssociateStateWithShaders(PipelineStateVariant* state, const HAL::RayTracingShaderBundle& shaders);

        void ConfigureDefaultStates();
        void BuildBaseRootSignature(); 

        ShaderManager* mShaderManager;
        RenderSurfaceDescription mDefaultRenderSurfaceDesc;
        
        HAL::Device* mDevice;
        HAL::RootSignature mBaseRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;

        std::unordered_map<PSOName, PipelineStateVariant> mPipelineStates;
        std::unordered_map<RootSignatureName, HAL::RootSignature> mRootSignatures;
        std::unordered_map<const HAL::Shader*, std::unordered_set<PipelineStateVariant*>> mShaderToPSOAssociations;
        std::unordered_set<PipelineStateVariant*> mStatesToRecompile;
    };

}
