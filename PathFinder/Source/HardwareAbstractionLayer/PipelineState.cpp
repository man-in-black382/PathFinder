#include "PipelineState.hpp"
#include "Utils.h"

#include "../Foundation/Assert.hpp"
#include "../Foundation/STDHelpers.hpp"
#include "../Foundation/StringUtils.hpp"

#include <d3d12.h>

namespace HAL
{

    PipelineState::PipelineState(const Device* device)
        : mDevice{ device } {}

    PipelineState::~PipelineState() {}

    void PipelineState::SetDebugName(const std::string& name)
    {
        if (mState)
        {
            mState->SetName(StringToWString(name).c_str());
        }

        mDebugName = name;
    }



    void GraphicsPipelineState::Compile()
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

        desc.pRootSignature = mRootSignature->D3DSignature();
        desc.VS = mVertexShader->D3DBytecode();
        desc.PS = mPixelShader->D3DBytecode();

        if (mDomainShader) desc.DS = mDomainShader->D3DBytecode();
        if (mHullShader) desc.HS = mHullShader->D3DBytecode();
        if (mGeometryShader) desc.GS = mGeometryShader->D3DBytecode();

        desc.BlendState = mBlendState.D3DState();
        desc.RasterizerState = mRasterizerState.D3DState();
        desc.DepthStencilState = mDepthStencilState.D3DState();
        desc.InputLayout = mInputLayout.D3DLayout();
        desc.PrimitiveTopologyType = D3DPrimitiveTopologyType(mPrimitiveTopology);
        desc.NumRenderTargets = (UINT)mRenderTargetFormats.size();
        desc.DSVFormat = ResourceFormat::D3DFormat(mDepthStencilFormat);
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleMask = 0xFFFFFFFF;
        
        for (auto& keyValue : mRenderTargetFormats)
        {
            auto rtIdx = std::underlying_type<RenderTarget>::type(keyValue.first);
            std::visit([&desc, rtIdx](auto&& format) {
                desc.RTVFormats[rtIdx] = ResourceFormat::D3DFormat(format);
            }, keyValue.second);
        }

        //desc.IBStripCutValue;
        //desc.SampleMask;
        //desc.StreamOutput;
        /*desc.NodeMask;
        desc.CachedPSO;*/

#if defined(DEBUG) || defined(_DEBUG) 
        //desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
        ThrowIfFailed(mDevice->D3DDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mState)));

        mState->SetName(StringToWString(mDebugName).c_str());
    }

    GraphicsPipelineState GraphicsPipelineState::Clone() const
    {
        GraphicsPipelineState newState = *this;
        newState.mState = nullptr;
        return newState;
    }

    void GraphicsPipelineState::ReplaceShader(const Shader* oldShader, const Shader* newShader)
    {
        if (mVertexShader == oldShader && newShader->PipelineStage() == Shader::Stage::Vertex) mVertexShader = newShader;
        else if (mPixelShader == oldShader && newShader->PipelineStage() == Shader::Stage::Pixel) mPixelShader = newShader;
        else if (mHullShader == oldShader && newShader->PipelineStage() == Shader::Stage::Hull) mHullShader = newShader;
        else if (mDomainShader == oldShader && newShader->PipelineStage() == Shader::Stage::Domain) mDomainShader = newShader;
        else if (mGeometryShader == oldShader && newShader->PipelineStage() == Shader::Stage::Geometry) mGeometryShader = newShader;
        else { assert_format(false, "Cannot find a shader to replace"); }
    }



    void ComputePipelineState::Compile()
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};

        desc.pRootSignature = mRootSignature->D3DSignature();
        desc.CS = mComputeShader->D3DBytecode();
        desc.NodeMask = 0;
        
#if defined(DEBUG) || defined(_DEBUG) 
        //desc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
        ThrowIfFailed(mDevice->D3DDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mState)));

        mState->SetName(StringToWString(mDebugName).c_str());
    }

    ComputePipelineState ComputePipelineState::Clone() const
    {
        ComputePipelineState newState = *this;
        newState.mState = nullptr;
        return newState;
    }

    void ComputePipelineState::ReplaceShader(const Shader* oldShader, const Shader* newShader)
    {
        assert_format(
            mComputeShader == oldShader && 
            newShader->PipelineStage() == Shader::Stage::Compute, 
            "Cannot find a shader to replace");

        mComputeShader = newShader;
    }

  
    //https://devblogs.nvidia.com/rtx-best-practices/
    // 
    // Consider using more than one ray tracing pipeline (State Object). 
    // This especially applies when you trace several types of rays,
    // such as shadows and reflections where one type (shadows) has a few simple shaders,
    // small payloads, and/or low register pressure, 
    // while the other type (reflections) involves many complex shaders and/or larger payloads.
    // Separating these cases into different pipelines helps the driver schedule
    // shader execution more efficiently and run workloads at higher occupancy.
    //
    RayTracingPipelineState::RayTracingPipelineState(const Device* device)
        : mDevice{ device }, mShaderConfig{ 4, 4 }, mPipelineConfig{ 1 } {}

    void RayTracingPipelineState::SetRayGenerationShader(const SingleRTShader& shader)
    {
        mRayGenerationShader = shader;
    }

    void RayTracingPipelineState::AddMissShader(const SingleRTShader& shader)
    {
        mMissShaders.push_back(shader);
    }

    void RayTracingPipelineState::AddHitGroupShaders(const HitGroupShaders& hgShaders)
    {
        assert_format(hgShaders.AnyHitShader || hgShaders.ClosestHitShader || hgShaders.IntersectionShader,
            "At least one hit group shader must exist");

        mHitGroupShaders.push_back(hgShaders);
    }

    void RayTracingPipelineState::SetPipelineConfig(const RayTracingPipelineConfig& config)
    {
        mPipelineConfig = config;
    }

    void RayTracingPipelineState::SetShaderConfig(const RayTracingShaderConfig& config)
    {
        mShaderConfig = config;
    }

    void RayTracingPipelineState::SetGlobalRootSignature(const RootSignature* signature)
    {
        mGlobalRootSignature = signature;
    }

    void RayTracingPipelineState::ReplaceShader(const Shader* oldShader, const Shader* newShader)
    {
        if (mRayGenerationShader.RTShader == oldShader && 
            newShader->PipelineStage() == Shader::Stage::RayGeneration)
        {
            mRayGenerationShader.RTShader = newShader;
            return;
        }

        if (newShader->PipelineStage() == Shader::Stage::RayMiss)
        {
            for (SingleRTShader& shader : mMissShaders)
            {
                if (shader.RTShader == oldShader)
                {
                    shader.RTShader = newShader;
                    return;
                }
            }
        }

        for (HitGroupShaders& shaders : mHitGroupShaders)
        {
            if (shaders.AnyHitShader == oldShader &&
                newShader->PipelineStage() == Shader::Stage::RayAnyHit)
            {
                shaders.AnyHitShader = newShader;
                return;
            }

            if (shaders.ClosestHitShader == oldShader && 
                newShader->PipelineStage() == Shader::Stage::RayClosestHit)
            {
                shaders.ClosestHitShader = newShader;
                return;
            }

            if (shaders.IntersectionShader == oldShader && 
                newShader->PipelineStage() == Shader::Stage::RayIntersection)
            {
                shaders.IntersectionShader = newShader;
                return;
            }
        }
    }

    void RayTracingPipelineState::Compile()
    {
        mLibraries.clear();
        mHitGroups.clear();
        mSubobjects.clear();
        mAssociations.clear();
        mExportNamePointerHolder.clear();

        GenerateLibrariesAndHitGroups();

        uint64_t subobjectsToReserve =
            mLibraries.size() * 5 + // Each Association can produce up to 5 subobjects
            mHitGroups.size() + // Each hit group produces 1 subobject
            2; // 1 global root sig subobject and 1 pipeline config subobject

        // Each Association can produce up to 2 association subobjects
        uint64_t associationsToReserve = mLibraries.size() * 2; 
            
        // Reserve vectors to avoid pointer invalidation on pushes and emplacements
        mSubobjects.reserve(subobjectsToReserve);
        mAssociations.reserve(associationsToReserve);
        mExportNamePointerHolder.reserve(associationsToReserve);

        for (const DXILLibrary& library : mLibraries)
        {
            AssociateLibraryWithItsExport(library);
            AssociateConfigWithExport(mShaderConfig, library.Export());

            if (library.LocalRootSignature())
            {
                AssociateRootSignatureWithExport(*library.LocalRootSignature(), library.Export());
            }
        }

        for (RayTracingHitGroup& hitGroup : mHitGroups)
        {
            AddHitGroupSubobject(hitGroup);
        }

        AddGlobalRootSignatureSubobject();
        AddPipelineConfigSubobject();

        mRTPSODesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        mRTPSODesc.NumSubobjects = (UINT)mSubobjects.size();
        mRTPSODesc.pSubobjects = mSubobjects.data();

        ThrowIfFailed(mDevice->D3DDevice()->CreateStateObject(&mRTPSODesc, IID_PPV_ARGS(mState.GetAddressOf())));
        ThrowIfFailed(mState->QueryInterface(IID_PPV_ARGS(mProperties.GetAddressOf())));

        mState->SetName(StringToWString(mDebugName).c_str());

        BuildShaderTable();
    }

    void RayTracingPipelineState::SetDebugName(const std::string& name)
    {
        if (mState)
        {
            mState->SetName(StringToWString(name).c_str());
        }

        mDebugName = name;
    }

    ShaderIdentifier RayTracingPipelineState::GetShaderIdentifier(const std::wstring& exportName)
    {
        uint8_t* rawID = reinterpret_cast<uint8_t*>(mProperties->GetShaderIdentifier(exportName.c_str()));
        ShaderIdentifier identifier;
        std::copy_n(rawID, identifier.RawData.size(), identifier.RawData.begin());
        return identifier;
    }

    std::wstring RayTracingPipelineState::GenerateUniqueExportName(const Shader& shader)
    {
        mUniqueShaderExportID++;
        return std::to_wstring(mUniqueShaderExportID) + shader.EntryPoint();
    }

    void RayTracingPipelineState::GenerateLibrariesAndHitGroups()
    {
        assert_format(mRayGenerationShader.RTShader, "Ray Generation shader is missing");

        GenerateLibrary(mRayGenerationShader.RTShader, mRayGenerationShader.LocalRootSignature);

        for (const SingleRTShader& missShader : mMissShaders)
        {
            GenerateLibrary(missShader.RTShader, missShader.LocalRootSignature);
        }

        for (const HitGroupShaders& shaders : mHitGroupShaders)
        {
            DXILLibrary* closestHitLib = nullptr;
            DXILLibrary* anyHitLib = nullptr;
            DXILLibrary* intersectionLib = nullptr;

            // Same local root signature for all hit group shaders
            if (shaders.ClosestHitShader) closestHitLib = &GenerateLibrary(shaders.ClosestHitShader, shaders.LocalRootSignature);
            if (shaders.AnyHitShader) anyHitLib = &GenerateLibrary(shaders.AnyHitShader, shaders.LocalRootSignature);
            if (shaders.IntersectionShader) intersectionLib = &GenerateLibrary(shaders.IntersectionShader, shaders.LocalRootSignature);

            assert_format(closestHitLib || anyHitLib || intersectionLib, "At least one hit group shader must exist");

            mHitGroups.emplace_back(&closestHitLib->Export(), &anyHitLib->Export(), &intersectionLib->Export(), shaders.LocalRootSignature);
        }
    }

    DXILLibrary& RayTracingPipelineState::GenerateLibrary(const Shader* shader, const RootSignature* localRootSignature)
    {
        ShaderExport shaderExport{ shader };
        shaderExport.SetExportName(GenerateUniqueExportName(*shader));
        DXILLibrary lib{ shaderExport };
        lib.SetLocalRootSignature(localRootSignature);
        return mLibraries.emplace_back(std::move(lib));
    }

    void RayTracingPipelineState::AssociateLibraryWithItsExport(const DXILLibrary& library)
    {
        mExportNamePointerHolder.emplace_back(library.Export().ExportName().c_str());

        // Hold in memory until compilation
        D3D12_STATE_SUBOBJECT& librarySubobject = 
            mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &library.D3DLibrary() });
    }

    void RayTracingPipelineState::AssociateConfigWithExport(const RayTracingShaderConfig& config, const ShaderExport& shaderExport)
    {
        mExportNamePointerHolder.emplace_back(shaderExport.ExportName().c_str());

        // Associate shader exports with shader config
        D3D12_STATE_SUBOBJECT& configSubobject = 
            mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &config.D3DConfig() });

        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& configToExportAssociation = 
            mAssociations.emplace_back(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION{ &configSubobject, 1, &mExportNamePointerHolder.back() });

        mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &configToExportAssociation });
    }

    void RayTracingPipelineState::AssociateRootSignatureWithExport(const RootSignature& signature, const ShaderExport& shaderExport)
    {
        mExportNamePointerHolder.emplace_back(shaderExport.ExportName().c_str());

        // Associate local root signatures with shader exports
        // Hold in memory until compilation
        D3D12_STATE_SUBOBJECT& localRootSignatureSubobject = 
            mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, signature.D3DSignature() });

        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& localRootSigToExportAssociation = 
            mAssociations.emplace_back(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION{ &localRootSignatureSubobject, 1, &mExportNamePointerHolder.back() });

        mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &localRootSigToExportAssociation });
    }

    void RayTracingPipelineState::AddHitGroupSubobject(const RayTracingHitGroup& group)
    {
        D3D12_STATE_SUBOBJECT hitGroupSubobject{ D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &group.D3DHitGroup() };
        mSubobjects.push_back(hitGroupSubobject);
    }

    void RayTracingPipelineState::AddGlobalRootSignatureSubobject()
    {
        if (!mGlobalRootSignature) return;

        mD3DGlobalRootSignanture.pGlobalRootSignature = mGlobalRootSignature->D3DSignature();

        D3D12_STATE_SUBOBJECT globalRootSignatureSubobject{ D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &mD3DGlobalRootSignanture };
        mSubobjects.push_back(globalRootSignatureSubobject);
    }

    void RayTracingPipelineState::AddPipelineConfigSubobject()
    {
        D3D12_STATE_SUBOBJECT configSubobject{ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &mPipelineConfig.D3DConfig() };
        mSubobjects.push_back(configSubobject);
    }

    void RayTracingPipelineState::BuildShaderTable()
    {
        mShaderTable.Clear();

        for (const DXILLibrary& library : mLibraries)
        {
            const ShaderExport& shaderExport = library.Export();
            Shader::Stage stage = shaderExport.AssosiatedShader()->PipelineStage();

            if (stage == Shader::Stage::RayGeneration)
            {
                ShaderIdentifier shaderId = GetShaderIdentifier(shaderExport.ExportName());
                mShaderTable.SetRayGenerationShader(shaderId, library.LocalRootSignature());
            }

            if (stage == Shader::Stage::RayMiss)
            {
                ShaderIdentifier shaderId = GetShaderIdentifier(shaderExport.ExportName());
                mShaderTable.AddRayMissShader(shaderId, library.LocalRootSignature());
            }
        }

        for (const RayTracingHitGroup& hitGroup : mHitGroups)
        {
            ShaderIdentifier shaderId = GetShaderIdentifier(hitGroup.ExportName());
            mShaderTable.AddRayTracingHitGroupShaders(shaderId, hitGroup.LocalRootSignature());
        }
    }

}
