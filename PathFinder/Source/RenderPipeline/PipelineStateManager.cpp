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
        AddCommonRootSignatureParameters(mBaseRootSignature);

        mSignaturesToCompile.insert(&mBaseRootSignature);

        mShaderManager->ShaderRecompilationEvent() += { "shader.recompilation", this, &PipelineStateManager::RecompileStatesWithNewShader };
        mShaderManager->LibraryRecompilationEvent() += { "library.recompilation", this, &PipelineStateManager::RecompileStatesWithNewLibrary };
    }

    void PipelineStateManager::CreateRootSignature(RootSignatureName name, const RootSignatureConfigurator& configurator)
    {
        assert_format(GetRootSignature(name) == nullptr, "Redefinition of Root Signature. ", name.ToString(), " already exists.");

        HAL::RootSignature newSignature{ mDevice };
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

        AddCommonRootSignatureParameters(newSignature);

        auto [iterator, success] = mRootSignatures.emplace(name, std::move(newSignature));
        mSignaturesToCompile.insert(&iterator->second);
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

        HAL::Shader* vertexShader = mShaderManager->LoadShader(HAL::Shader::Stage::Vertex, mDefaultVertexEntryPointName, proxy.VertexShaderFileName);
        HAL::Shader* pixelShader = mShaderManager->LoadShader(HAL::Shader::Stage::Pixel, mDefaultPixelEntryPointName, proxy.PixelShaderFileName);
        HAL::Shader* geometryShader = proxy.GeometryShaderFileName ? mShaderManager->LoadShader(HAL::Shader::Stage::Geometry, mDefaultGeometryEntryPointName, *proxy.GeometryShaderFileName) : nullptr;
        
        newState.SetVertexShader(vertexShader);
        newState.SetPixelShader(pixelShader);

        if (geometryShader) newState.SetGeometryShader(geometryShader);

        newState.SetRootSignature(GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetDebugName(name.ToString());

        auto [iter, success] = mPipelineStates.emplace(name, std::move(newState));
        mStatesToCompile.insert(&iter->second);

        AssociateStateWithShader(&iter->second, vertexShader);
        AssociateStateWithShader(&iter->second, pixelShader);

        if (geometryShader) AssociateStateWithShader(&iter->second, geometryShader);
    }

    void PipelineStateManager::CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator)
    {
        assert_format(GetPipelineState(name) == std::nullopt, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        ComputeStateProxy proxy{};
        configurator(proxy);

        assert_format(!proxy.ComputeShaderFileName.empty(), "Compute shader is missing");

        HAL::ComputePipelineState newState{ mDevice };

        HAL::Shader* computeShader = mShaderManager->LoadShader(HAL::Shader::Stage::Compute, mDefaultComputeEntryPointName, proxy.ComputeShaderFileName);

        newState.SetRootSignature(GetNamedRootSignatureOrDefault(proxy.RootSignatureName));
        newState.SetComputeShader(computeShader);
        newState.SetDebugName(name.ToString());

        auto [iter, success] = mPipelineStates.emplace(name, std::move(newState));
        mStatesToCompile.insert(&iter->second);

        AssociateStateWithShader(&iter->second, computeShader);
    }

    void PipelineStateManager::CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator)
    {
        assert_format(GetPipelineState(name) == std::nullopt, "Redefinition of pipeline state. ", name.ToString(), " already exists.");

        RayTracingStateProxy proxy{};
        configurator(proxy);

        assert_format(!proxy.RayGenerationShaderFileName.empty(), "Ray generation shader is missing");
        assert_format(proxy.PipelineConfig.MaxTracingRecursionDepth() > 0, "Pipeline config was not set");
        assert_format(proxy.ShaderConfig.MaxPayloadSize() > 0, "Shader config payload size was not set");
        // This one is especially important because values less than *8* can cause driver crashes on Nvidia when callable shaders are used,
        // but DX validation layer will not say anything. Driver version 442.74.
        assert_format(proxy.ShaderConfig.MaxAttributesSize() >= 8, "Shader config attributes size was not set to proper value"); 

        auto [iter, success] = mPipelineStates.emplace(name, RayTracingStateWrapper{ name, HAL::RayTracingPipelineState{ mDevice }, nullptr });
        mStatesToCompile.insert(&iter->second);

        RayTracingStateWrapper& newStateWrapper = std::get<RayTracingStateWrapper>(iter->second);

        HAL::Library* rayGenLibrary = mShaderManager->LoadLibrary(proxy.RayGenerationShaderFileName);
        newStateWrapper.State.SetRayGenerationShader({ rayGenLibrary, mDefaultRayGenerationEntryPointName, GetNamedRootSignatureOrNull(proxy.RayGenerationLocalRootSignatureName) });
        AssociateStateWithLibrary(&iter->second, rayGenLibrary);

        for (const RayTracingStateProxy::MissShader& missShaderInfo : proxy.MissShaders())
        {
            HAL::Library* missLibrary = mShaderManager->LoadLibrary(missShaderInfo.MissShaderFileName);
            newStateWrapper.State.AddMissShader({ missLibrary, mDefaultRayMissEntryPointName, GetNamedRootSignatureOrNull(missShaderInfo.LocalRootSignatureName) });
            AssociateStateWithLibrary(&iter->second, missLibrary);
        }

        for (const RayTracingStateProxy::CallableShader& callableShaderInfo : proxy.CallableShaders())
        {
            HAL::Library* callableLibrary = mShaderManager->LoadLibrary(callableShaderInfo.ShaderFileName);
            newStateWrapper.State.AddCallableShader({ callableLibrary, callableShaderInfo.EntryPointName, GetNamedRootSignatureOrNull(callableShaderInfo.LocalRootSignatureName) });
            AssociateStateWithLibrary(&iter->second, callableLibrary);
        }

        for (const RayTracingStateProxy::HitGroup& hitGroupInfo : proxy.HitGroups())
        {
            HAL::RayTracingPipelineState::HitGroupShaders shaders{};

            if (auto fileName = hitGroupInfo.ShaderFileNames.AnyHitShaderFileName)
            {
                shaders.AnyHitLibrary = mShaderManager->LoadLibrary(*fileName);
                shaders.AnyHitEntryPoint = mDefaultRayAnyHitEntryPointName;
                AssociateStateWithLibrary(&iter->second, shaders.AnyHitLibrary);
            }

            if (auto fileName = hitGroupInfo.ShaderFileNames.ClosestHitShaderFileName)
            {
                shaders.ClosestHitLibrary = mShaderManager->LoadLibrary(*fileName);
                shaders.ClosestHitEntryPoint = mDefaultRayClosestHitEntryPointName;
                AssociateStateWithLibrary(&iter->second, shaders.ClosestHitLibrary);
            }

            if (auto fileName = hitGroupInfo.ShaderFileNames.IntersectionShaderFileName)
            {
                shaders.IntersectionLibrary = mShaderManager->LoadLibrary(*fileName);
                shaders.IntersectionEntryPoint = mDefaultRayIntersectionEntryPointName;
                AssociateStateWithLibrary(&iter->second, shaders.IntersectionLibrary);
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

    void PipelineStateManager::CompileUncompiledSignaturesAndStates()
    {
        for (HAL::RootSignature* signature : mSignaturesToCompile)
        {
            signature->Compile();
        }

        for (PipelineStateVariantInternal* state : mStatesToCompile)
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
                CompileRayTracingState(*psoWrapper);
            }
        }

        mSignaturesToCompile.clear();
        mStatesToCompile.clear();
    }

    void PipelineStateManager::AssociateStateWithShader(PipelineStateVariantInternal* state, const HAL::Shader* shader)
    {
        mShaderToPSOAssociations[shader].insert(state);
    }

    void PipelineStateManager::AssociateStateWithLibrary(PipelineStateVariantInternal* state, const HAL::Library* library)
    {
        mLibraryToPSOAssociations[library].insert(state);
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

    void PipelineStateManager::AddCommonRootSignatureParameters(HAL::RootSignature& signature) const
    {
        // Global data CB
        signature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 0, 10 });

        // Frame-specific data CB
        signature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 1, 10 });

        // Pass-specific data CB
        signature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 2, 10 });

        // Unbounded Texture2D range
        HAL::RootDescriptorTableParameter textures2D;
        textures2D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 10 });
        signature.AddDescriptorTableParameter(textures2D);

        // Unbounded Texture2D<uint4> range
        HAL::RootDescriptorTableParameter textures2DUInt4;
        textures2DUInt4.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 11 });
        signature.AddDescriptorTableParameter(textures2DUInt4);

        // Unbounded Texture3D range
        HAL::RootDescriptorTableParameter textures3D;
        textures3D.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 12 });
        signature.AddDescriptorTableParameter(textures3D);

        // Unbounded Texture3D<uint4> range
        HAL::RootDescriptorTableParameter textures3DUInt4;
        textures3DUInt4.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 13 });
        signature.AddDescriptorTableParameter(textures3DUInt4);

        // Unbounded Texture2DArray range
        HAL::RootDescriptorTableParameter texture2DArrays;
        texture2DArrays.AddDescriptorRange(HAL::SRDescriptorTableRange{ 0, 14 });
        signature.AddDescriptorTableParameter(texture2DArrays);

        // Unbounded RWTexture2D range
        HAL::RootDescriptorTableParameter RWTextures2DFloat4;
        RWTextures2DFloat4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 10 });
        signature.AddDescriptorTableParameter(RWTextures2DFloat4);

        // Unbounded RWTexture2D<uint4> range
        HAL::RootDescriptorTableParameter RWTextures2DUInt4;
        RWTextures2DUInt4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 11 });
        signature.AddDescriptorTableParameter(RWTextures2DUInt4);

        // Unbounded RWTexture3D range
        HAL::RootDescriptorTableParameter RWTextures3D;
        RWTextures3D.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 12 });
        signature.AddDescriptorTableParameter(RWTextures3D);

        // Unbounded RWTexture3D<uint4> range
        HAL::RootDescriptorTableParameter RWTextures3DUInt4;
        RWTextures3DUInt4.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 13 });
        signature.AddDescriptorTableParameter(RWTextures3DUInt4);

        // Unbounded RWTexture2DArray range
        HAL::RootDescriptorTableParameter RWTexture2DArrays;
        RWTexture2DArrays.AddDescriptorRange(HAL::UADescriptorTableRange{ 0, 14 });
        signature.AddDescriptorTableParameter(RWTexture2DArrays);

        signature.AddStaticSampler(HAL::StaticSampler::AnisotropicClamp(0));
        signature.AddStaticSampler(HAL::StaticSampler::LinearClamp(1));
        signature.AddStaticSampler(HAL::StaticSampler::PointClamp(2));

        // Debug readback buffer
        HAL::RootUnorderedAccessParameter debugBuffer{ 0, 15 };
        signature.AddDescriptorParameter(debugBuffer);
    }

    void PipelineStateManager::CompileRayTracingState(RayTracingStateWrapper& stateWrapper)
    {
        stateWrapper.State.Compile();
        HAL::ShaderTable& shaderTable = stateWrapper.State.GetShaderTable();
        HAL::BufferProperties properties{ shaderTable.GetMemoryRequirements().TableSizeInBytes };
        stateWrapper.ShaderTableBuffer = mResourceProducer->NewBuffer(properties);
        stateWrapper.ShaderTableBuffer->RequestWrite();
        stateWrapper.BaseRayDispatchInfo = shaderTable.UploadToGPUMemory(stateWrapper.ShaderTableBuffer->WriteOnlyPtr(), stateWrapper.ShaderTableBuffer->HALBuffer());
        stateWrapper.ShaderTableBuffer->SetDebugName(StringFormat("%s Shader Table", stateWrapper.Name.ToString().c_str()));
    }

    void PipelineStateManager::RecompileStatesWithNewShader(const HAL::Shader* oldShader, const HAL::Shader* newShader)
    {
        auto associationsIt = mShaderToPSOAssociations.find(oldShader);
        assert_format(associationsIt != mShaderToPSOAssociations.end(), "Cannot find PSOs after shader recompilation");

        auto& statePtrs = associationsIt->second;

        for (PipelineStateVariantInternal* stateVariant : statePtrs)
        {
            if (auto pso = std::get_if<HAL::GraphicsPipelineState>(stateVariant)) pso->ReplaceShader(oldShader, newShader);
            else if (auto pso = std::get_if<HAL::ComputePipelineState>(stateVariant)) pso->ReplaceShader(oldShader, newShader);

            mStatesToCompile.insert(stateVariant);
        }

        // Re associate states 
        mShaderToPSOAssociations[newShader] = std::move(associationsIt->second);
        mShaderToPSOAssociations.erase(associationsIt);
    }

    void PipelineStateManager::RecompileStatesWithNewLibrary(const HAL::Library* oldLibrary, const HAL::Library* newLibrary)
    {
        auto associationsIt = mLibraryToPSOAssociations.find(oldLibrary);
        assert_format(associationsIt != mLibraryToPSOAssociations.end(), "Cannot find PSOs after library recompilation");

        auto& statePtrs = associationsIt->second;

        for (PipelineStateVariantInternal* stateVariant : statePtrs)
        {
            if (auto psoWrapper = std::get_if<RayTracingStateWrapper>(stateVariant))
            {
                psoWrapper->State.ReplaceLibrary(oldLibrary, newLibrary);
            }
            mStatesToCompile.insert(stateVariant);
        }

        // Re associate states 
        mLibraryToPSOAssociations[newLibrary] = std::move(associationsIt->second);
        mLibraryToPSOAssociations.erase(associationsIt);
    }

}