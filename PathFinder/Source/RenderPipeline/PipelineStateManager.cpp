#include "PipelineStateManager.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineStateManager::PipelineStateManager(
        HAL::Device* device,
        ShaderManager* shaderManager, 
        Memory::GPUResourceProducer* resourceProducer,
        const RenderSurfaceDescription& defaultRenderSurface)
        : 
        mDevice{ device }, 
        mShaderManager{ shaderManager },
        mResourceProducer{ resourceProducer },
        mDefaultRenderSurfaceDesc{ defaultRenderSurface }, 
        mBaseRootSignature{ device },
        mDefaultGraphicsState{ device }
    {
        ConfigureDefaultStates();
        BuildBaseRootSignature();

        mShaderManager->SetShaderRecompilationCallback([this](const HAL::Shader* oldShader, const HAL::Shader* newShader) 
        {
            auto associationsIt = mShaderToPSOAssociations.find(oldShader);
            assert_format(associationsIt != mShaderToPSOAssociations.end(), "Cannot find PSOs after shader recompilation");

            auto& statePtrs = associationsIt->second;

            for (PipelineStateVariantInternal* stateVariant : statePtrs)
            {
                if (auto pso = std::get_if<HAL::GraphicsPipelineState>(stateVariant)) pso->ReplaceShader(oldShader, newShader);
                else if (auto pso = std::get_if<HAL::ComputePipelineState>(stateVariant)) pso->ReplaceShader(oldShader, newShader);
                else if (auto psoWrapper = std::get_if<RayTracingStateWrapper>(stateVariant)) psoWrapper->State.ReplaceShader(oldShader, newShader);
                    
                mStatesToRecompile.insert(stateVariant);
            }

            // Re associate states 
            auto nodeHandle = mShaderToPSOAssociations.extract(associationsIt);
            nodeHandle.key() = newShader;
            mShaderToPSOAssociations.insert(std::move(nodeHandle));
        });
    }

    void PipelineStateManager::CreateRootSignature(RootSignatureName name, const RootSignatureConfigurator& configurator)
    {
        assert_format(GetRootSignature(name) == nullptr, "Redefinition of Root Signature. ", name.ToString(), " already exists.");

        HAL::RootSignature newSignature = mBaseRootSignature.Clone();
        RootSignatureProxy signatureProxy{};
        configurator(signatureProxy);

        for (const HAL::RootConstantsParameter& parameter : signatureProxy.RootConstantsParameters())
        {
            newSignature.AddConstantsParameter(parameter);
        }

        for (const HAL::RootConstantBufferParameter& parameter : signatureProxy.RootConstantBufferParameters())
        {
            newSignature.AddDescriptorParameter(parameter);
        }

        for (const HAL::RootDescriptorTableParameter& parameter : signatureProxy.RootDescriptorTableParameters())
        {
            newSignature.AddDescriptorTableParameter(parameter);
        }

        mRootSignatures.emplace(name, std::move(newSignature));
    }

    void PipelineStateManager::CreateGraphicsState(PSOName name, const GraphicsStateConfigurator& configurator)
    {
        assert_format(GetPipelineState(name) == std::nullopt, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        GraphicsStateProxy proxy{};

        const HAL::GraphicsPipelineState* defaultGraphicsState = &mDefaultGraphicsState;

        proxy.BlendState = defaultGraphicsState->GetBlendState();
        proxy.RasterizerState = defaultGraphicsState->GetRasterizerState();
        proxy.DepthStencilState = defaultGraphicsState->GetDepthStencilState();
        proxy.DepthStencilFormat = mDefaultRenderSurfaceDesc.DepthStencilFormat();
        proxy.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;

        configurator(proxy);

        assert_format(!proxy.VertexShaderFileName.empty(), "Vertex shader is missing");
        assert_format(!proxy.PixelShaderFileName.empty(), "Pixel shader is missing");

        if (proxy.RenderTargetFormats.empty())
        {
            proxy.RenderTargetFormats.push_back(mDefaultRenderSurfaceDesc.RenderTargetFormat());
        }

        HAL::GraphicsPipelineState newState = mDefaultGraphicsState.Clone();

        newState.SetBlendState(proxy.BlendState);
        newState.SetRasterizerState(proxy.RasterizerState);
        newState.SetDepthStencilState(proxy.DepthStencilState);
        newState.SetDepthStencilFormat(proxy.DepthStencilFormat);
        newState.SetPrimitiveTopology(proxy.PrimitiveTopology);

        // Vertex shaders are getting more and more unified. PathFinder is not using
        // vertex input buffers since they're just normal structured buffers under the hood.
        // Use structured buffers directly in vertex shader to get vertices/indices.
        // Set dummy to avoid runtime errors.
        newState.SetInputAssemblerLayout(HAL::InputAssemblerLayout{});

        newState.SetRenderTargetFormats(
            proxy.RenderTargetFormats.size() > 0 ? std::optional(proxy.RenderTargetFormats[0]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 1 ? std::optional(proxy.RenderTargetFormats[1]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 2 ? std::optional(proxy.RenderTargetFormats[2]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 3 ? std::optional(proxy.RenderTargetFormats[3]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 4 ? std::optional(proxy.RenderTargetFormats[4]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 5 ? std::optional(proxy.RenderTargetFormats[5]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 6 ? std::optional(proxy.RenderTargetFormats[6]) : std::nullopt,
            proxy.RenderTargetFormats.size() > 7 ? std::optional(proxy.RenderTargetFormats[7]) : std::nullopt
        );

        HAL::Shader* vertexShader = mShaderManager->LoadShader(HAL::Shader::Stage::Vertex, proxy.VertexShaderFileName);
        HAL::Shader* pixelShader = mShaderManager->LoadShader(HAL::Shader::Stage::Pixel, proxy.PixelShaderFileName);
        HAL::Shader* geometryShader = proxy.GeometryShaderFileName ? mShaderManager->LoadShader(HAL::Shader::Stage::Geometry, *proxy.GeometryShaderFileName) : nullptr;
        
        newState.SetVertexShader(vertexShader);
        newState.SetPixelShader(pixelShader);

        if (geometryShader) newState.SetGeometryShader(geometryShader);

        newState.SetRootSignature(GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetDebugName(name.ToString());

        auto [iter, success] = mPipelineStates.emplace(name, std::move(newState));

        AssociateStateWithShader(&iter->second, vertexShader);
        AssociateStateWithShader(&iter->second, vertexShader);

        if (geometryShader) AssociateStateWithShader(&iter->second, geometryShader);
    }

    void PipelineStateManager::CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator)
    {
        assert_format(GetPipelineState(name) == std::nullopt, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        ComputeStateProxy proxy{};
        configurator(proxy);

        assert_format(!proxy.ComputeShaderFileName.empty(), "Compute shader is missing");

        HAL::ComputePipelineState newState{ mDevice };

        HAL::Shader* computeShader = mShaderManager->LoadShader(HAL::Shader::Stage::Compute, proxy.ComputeShaderFileName);

        newState.SetRootSignature(GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetComputeShader(computeShader);
        newState.SetDebugName(name.ToString());

        auto [iter, success] = mPipelineStates.emplace(name, std::move(newState));

        AssociateStateWithShader(&iter->second, computeShader);
    }

    void PipelineStateManager::CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator)
    {
        assert_format(GetPipelineState(name) == std::nullopt, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        RayTracingStateProxy proxy{};
        configurator(proxy);

        assert_format(!proxy.RayGenerationShaderFileName.empty(), "Ray generation shader is missing");
        assert_format(proxy.PipelineConfig.MaxTracingRecursionDepth() > 0, "Pipeline config was not set");
        assert_format(proxy.ShaderConfig.MaxPayloadSize() > 0, "Shader config was not set");

        auto [iter, success] = mPipelineStates.emplace(name, RayTracingStateWrapper{ HAL::RayTracingPipelineState{ mDevice }, nullptr });
        RayTracingStateWrapper& newStateWrapper = std::get<RayTracingStateWrapper>(iter->second);

        HAL::Shader* rayGenShader = mShaderManager->LoadShader(HAL::Shader::Stage::RayGeneration, proxy.RayGenerationShaderFileName);
        newStateWrapper.State.SetRayGenerationShader({ rayGenShader, GetNamedRootSignatureOrNull(proxy.RayGenerationLocalRootSignatureName) });
        AssociateStateWithShader(&iter->second, rayGenShader);

        for (const RayTracingStateProxy::MissShader& missShaderInfo : proxy.MissShaders())
        {
            HAL::Shader* missShader = mShaderManager->LoadShader(HAL::Shader::Stage::RayMiss, missShaderInfo.MissShaderFileName);
            newStateWrapper.State.AddMissShader({ missShader, GetNamedRootSignatureOrNull(missShaderInfo.LocalRootSignatureName) });
            AssociateStateWithShader(&iter->second, missShader);
        }

        for (const RayTracingStateProxy::HitGroup& hitGroupInfo : proxy.HitGroups())
        {
            HAL::RayTracingPipelineState::HitGroupShaders shaders{};

            if (auto fileName = hitGroupInfo.ShaderFileNames.AnyHitShaderFileName)
            {
                shaders.AnyHitShader = mShaderManager->LoadShader(HAL::Shader::Stage::RayAnyHit, *fileName);
                AssociateStateWithShader(&iter->second, shaders.AnyHitShader);
            }

            if (auto fileName = hitGroupInfo.ShaderFileNames.ClosestHitShaderFileName)
            {
                shaders.ClosestHitShader = mShaderManager->LoadShader(HAL::Shader::Stage::RayClosestHit, *fileName);
                AssociateStateWithShader(&iter->second, shaders.ClosestHitShader);
            }

            if (auto fileName = hitGroupInfo.ShaderFileNames.IntersectionShaderFileName)
            {
                shaders.IntersectionShader = mShaderManager->LoadShader(HAL::Shader::Stage::RayIntersection, *fileName);
                AssociateStateWithShader(&iter->second, shaders.IntersectionShader);
            }

            shaders.LocalRootSignature = GetNamedRootSignatureOrNull(hitGroupInfo.LocalRootSignatureName);

            newStateWrapper.State.AddHitGroupShaders(shaders);
        }

        newStateWrapper.State.SetPipelineConfig(proxy.PipelineConfig);
        newStateWrapper.State.SetShaderConfig(proxy.ShaderConfig);
        newStateWrapper.State.SetGlobalRootSignature(GetNamedRootSignatureOrDefault(proxy.GlobalRootSignatureName));
        newStateWrapper.State.SetDebugName(name.ToString());
    }

    std::optional<PipelineStateManager::PipelineStateVariant> PipelineStateManager::GetPipelineState(PSOName name) const
    {
        auto it = mPipelineStates.find(name);
        if (it == mPipelineStates.end()) return std::nullopt;

        if (auto pso = std::get_if<HAL::GraphicsPipelineState>(&it->second)) return { pso };
        else if (auto pso = std::get_if<HAL::ComputePipelineState>(&it->second)) return { pso };
        else if (auto psoWrapper = std::get_if<RayTracingStateWrapper>(&it->second)) return PipelineStateVariant{ &psoWrapper->State, &(*psoWrapper->BaseRayDispatchInfo) };

        return std::nullopt;
    }

    const HAL::RootSignature* PipelineStateManager::GetRootSignature(RootSignatureName name) const
    {
        auto it = mRootSignatures.find(name);
        if (it == mRootSignatures.end()) return nullptr;
        return &it->second;
    }

    const HAL::RootSignature* PipelineStateManager::GetNamedRootSignatureOrDefault(std::optional<RootSignatureName> name) const
    {
        if (!name) return &mBaseRootSignature;

        const HAL::RootSignature* signature = GetRootSignature(*name);
        assert_format(signature, "Root signature ", name->ToString(), " doesn't exist");
        return signature;
    }

    const HAL::RootSignature* PipelineStateManager::GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const
    {
        if (!name) return nullptr;

        const HAL::RootSignature* signature = GetRootSignature(*name);
        assert_format(signature, "Root signature ", name->ToString(), " doesn't exist");
        return signature;
    }

    const HAL::RootSignature& PipelineStateManager::BaseRootSignature() const
    {
        return mBaseRootSignature;
    }

    void PipelineStateManager::CompileSignaturesAndStates()
    {
        mBaseRootSignature.Compile();

        for (auto& [name, signature] : mRootSignatures)
        {
            signature.Compile();
        }

        for (auto& [name, state] : mPipelineStates)
        {
            if (auto pso = std::get_if<HAL::GraphicsPipelineState>(&state))
            {
                pso->Compile();
            }
            else if (auto pso = std::get_if<HAL::ComputePipelineState>(&state))
            {
                pso->Compile();
            }
            else if (auto psoWrapper = std::get_if<RayTracingStateWrapper>(&state))
            {
                CompileRayTracingState(*psoWrapper, name);
            }
        }
    }

    void PipelineStateManager::RecompileModifiedStates()
    {
        for (PipelineStateVariantInternal* state : mStatesToRecompile)
        {
            if (auto pso = std::get_if<HAL::GraphicsPipelineState>(state))
            { 
                pso->Compile();
            }
            else if (auto pso = std::get_if<HAL::ComputePipelineState>(state))
            {
                pso->Compile(); 
            }
            else if (auto psoWrapper = std::get_if<RayTracingStateWrapper>(state))
            {
                CompileRayTracingState(*psoWrapper, {});
            }
        }

        mStatesToRecompile.clear();
    }

    bool PipelineStateManager::HasModifiedStates() const
    {
        return !mStatesToRecompile.empty();
    }

    void PipelineStateManager::AssociateStateWithShader(PipelineStateVariantInternal* state, const HAL::Shader* shader)
    {
        mShaderToPSOAssociations[shader].insert(state);
    }

    void PipelineStateManager::ConfigureDefaultStates()
    {
        mDefaultGraphicsState.GetBlendState().SetBlendingEnabled(false);
        mDefaultGraphicsState.GetDepthStencilState().SetDepthTestEnabled(true);
        mDefaultGraphicsState.GetDepthStencilState().SetDepthWriteEnabled(true);
        mDefaultGraphicsState.SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
        mDefaultGraphicsState.GetRasterizerState().SetCullMode(HAL::RasterizerState::CullMode::Back);
        mDefaultGraphicsState.GetRasterizerState().SetFillMode(HAL::RasterizerState::FillMode::Solid);
        mDefaultGraphicsState.SetDepthStencilFormat(mDefaultRenderSurfaceDesc.DepthStencilFormat());
        mDefaultGraphicsState.SetRootSignature(&mBaseRootSignature);
    }

    void PipelineStateManager::BuildBaseRootSignature()
    {
        // See BaseRootSignature.hlsl for reference

        // Global data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 0, 10 }); 

        // Frame-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 1, 10 });

        // Pass-specific data CB
        mBaseRootSignature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 2, 10 });

        // Unbounded Texture2D range
        HAL::RootDescriptorTableParameter textures2D;
        textures2D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 10 });
        mBaseRootSignature.AddDescriptorTableParameter(textures2D);

        // Unbounded Texture2D<uint4> range
        HAL::RootDescriptorTableParameter textures2DUInt4;
        textures2DUInt4.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 11 });
        mBaseRootSignature.AddDescriptorTableParameter(textures2DUInt4);

        // Unbounded Texture3D range
        HAL::RootDescriptorTableParameter textures3D;
        textures3D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 12 }); 
        mBaseRootSignature.AddDescriptorTableParameter(textures3D);

        // Unbounded Texture3D<uint4> range
        HAL::RootDescriptorTableParameter textures3DUInt4;
        textures3DUInt4.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 13 });
        mBaseRootSignature.AddDescriptorTableParameter(textures3DUInt4);

        // Unbounded Texture2DArray range
        HAL::RootDescriptorTableParameter texture2DArrays;
        texture2DArrays.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 14 });
        mBaseRootSignature.AddDescriptorTableParameter(texture2DArrays);

        // Unbounded RWTexture2D range
        HAL::RootDescriptorTableParameter RWTextures2DFloat4;
        RWTextures2DFloat4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 10 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures2DFloat4);

        // Unbounded RWTexture2D<uint4> range
        HAL::RootDescriptorTableParameter RWTextures2DUInt4;
        RWTextures2DUInt4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 11 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures2DUInt4);

        // Unbounded RWTexture3D range
        HAL::RootDescriptorTableParameter RWTextures3D;
        RWTextures3D.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 12 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures3D);

        // Unbounded RWTexture3D<uint4> range
        HAL::RootDescriptorTableParameter RWTextures3DUInt4;
        RWTextures3DUInt4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 13 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTextures3DUInt4);

        // Unbounded RWTexture2DArray range
        HAL::RootDescriptorTableParameter RWTexture2DArrays;
        RWTexture2DArrays.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 14 });
        mBaseRootSignature.AddDescriptorTableParameter(RWTexture2DArrays);

        mBaseRootSignature.AddStaticSampler(HAL::StaticSampler::AnisotropicClamp(0));
        mBaseRootSignature.AddStaticSampler(HAL::StaticSampler::LinearClamp(1));
        mBaseRootSignature.AddStaticSampler(HAL::StaticSampler::PointClamp(2));

        // Debug readback buffer
        HAL::RootUnorderedAccessParameter debugBuffer{ 0, 15 };
        mBaseRootSignature.AddDescriptorParameter(debugBuffer);
    }

    void PipelineStateManager::CompileRayTracingState(RayTracingStateWrapper& stateWrapper, Foundation::Name psoName)
    {
        stateWrapper.State.Compile();
        HAL::ShaderTable& shaderTable = stateWrapper.State.GetShaderTable();
        HAL::Buffer::Properties properties{ shaderTable.GetMemoryRequirements().TableSizeInBytes };
        stateWrapper.ShaderTableBuffer = mResourceProducer->NewBuffer(properties);
        stateWrapper.ShaderTableBuffer->RequestWrite();
        stateWrapper.BaseRayDispatchInfo = shaderTable.UploadToGPUMemory(stateWrapper.ShaderTableBuffer->WriteOnlyPtr(), stateWrapper.ShaderTableBuffer->HALBuffer());

        if (psoName.IsValid())
        {
            stateWrapper.ShaderTableBuffer->SetDebugName(StringFormat("%s Shader Table", psoName.ToString().c_str()));
        }
    }

}