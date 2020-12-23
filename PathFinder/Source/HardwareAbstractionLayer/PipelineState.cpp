#include "PipelineState.hpp"
#include "Utils.h"

#include <Foundation/STDHelpers.hpp>
#include <Foundation/StringUtils.hpp>

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
        desc.DSVFormat = D3DFormat(mDepthStencilFormat);
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleMask = 0xFFFFFFFF;
        desc.NodeMask = mDevice->NodeMask();
        
        for (auto& keyValue : mRenderTargetFormats)
        {
            auto rtIdx = std::underlying_type<RenderTarget>::type(keyValue.first);
            std::visit([&desc, rtIdx](auto&& format) {
                desc.RTVFormats[rtIdx] = D3DFormat(format);
            }, keyValue.second);
        }

        //desc.IBStripCutValue;
        //desc.SampleMask;
        //desc.StreamOutput;
        //desc.CachedPSO;

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
        desc.NodeMask = mDevice->NodeMask();
        
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
        mRayGenerationExport = GenerateLibraryExport(shader.EntryPoint);
    }

    void RayTracingPipelineState::AddMissShader(const SingleRTShader& shader)
    {
        mMissShaders.push_back(shader);
        mMissShaderExports.push_back(GenerateLibraryExport(shader.EntryPoint));
    }

    void RayTracingPipelineState::AddCallableShader(const SingleRTShader& shader)
    {
        mCallableShaders.push_back(shader);
        mCallableShaderExports.push_back(GenerateLibraryExport(shader.EntryPoint));
    }

    void RayTracingPipelineState::AddHitGroupShaders(const HitGroupShaders& hgShaders)
    {
        assert_format(hgShaders.AnyHitLibrary || hgShaders.ClosestHitLibrary || hgShaders.IntersectionLibrary,
            "At least one hit group shader must exist");

        mHitGroupShaders.push_back(hgShaders);

        HitGroupLibraryExports hgExports{
            hgShaders.AnyHitLibrary ? std::optional(GenerateLibraryExport(hgShaders.AnyHitEntryPoint)) : std::nullopt,
            hgShaders.ClosestHitLibrary ? std::optional(GenerateLibraryExport(hgShaders.ClosestHitEntryPoint)) : std::nullopt,
            hgShaders.IntersectionLibrary ? std::optional(GenerateLibraryExport(hgShaders.IntersectionEntryPoint)) : std::nullopt
        };

        mHitGroupExports.push_back(hgExports);
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

    void RayTracingPipelineState::ReplaceLibrary(const Library* oldLibrary, const Library* newLibrary)
    {
        if (mRayGenerationShader.ShaderLibrary == oldLibrary)
        {
            mRayGenerationShader.ShaderLibrary = newLibrary;
        }

        for (SingleRTShader& missShader : mMissShaders)
        {
            if (missShader.ShaderLibrary == oldLibrary)
            {
                missShader.ShaderLibrary = newLibrary;
            }
        }

        for (SingleRTShader& callableShader : mCallableShaders)
        {
            if (callableShader.ShaderLibrary == oldLibrary)
            {
                callableShader.ShaderLibrary = newLibrary;
            }
        }

        for (HitGroupShaders& hgShaders : mHitGroupShaders)
        {
            if (hgShaders.AnyHitLibrary == oldLibrary) hgShaders.AnyHitLibrary = newLibrary;
            if (hgShaders.ClosestHitLibrary == oldLibrary) hgShaders.ClosestHitLibrary = newLibrary;
            if (hgShaders.IntersectionLibrary == oldLibrary) hgShaders.IntersectionLibrary = newLibrary;
        }
    }

    void RayTracingPipelineState::Compile()
    {
        mLibraryExportCollections.clear();
        mHitGroups.clear();
        mSubobjects.clear();
        mAssociations.clear();
        mExportNamePointerHolder.clear();

        GenerateLibraryExportCollectionsAndHitGroups();

        uint64_t subobjectCountPerShader = 6;
        uint64_t associationCountPerShader = 2;

        uint64_t subobjectsToReserve =
            subobjectCountPerShader + 
            mMissShaders.size() * subobjectCountPerShader + 
            mCallableShaders.size() * subobjectCountPerShader + 
            mHitGroupShaders.size() * subobjectCountPerShader + // Each Association can produce up to 6 subobjects
            mHitGroups.size() + // Each hit group produces 1 subobject
            2; // 1 global root sig subobject and 1 pipeline config subobject

        // Each Association can produce up to 2 association subobjects
        uint64_t associationsToReserve = 
            associationCountPerShader + 
            mMissShaders.size() * associationCountPerShader +
            mCallableShaders.size() * associationCountPerShader +
            mHitGroupShaders.size() * associationCountPerShader;

        // Reserve vectors to avoid pointer invalidation on pushes and emplacements
        mSubobjects.reserve(subobjectsToReserve);
        mAssociations.reserve(associationsToReserve);
        mExportNamePointerHolder.reserve(associationsToReserve);

        AddLibraryExportCollectionSubobjects();
        AssociateExportsWithLocalRootSignaturesAndShaderConfig();
        AddHitGroupSubobjects();
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

    HAL::LibraryExport RayTracingPipelineState::GenerateLibraryExport(const std::string& shaderEntryPoint)
    {
        LibraryExport libExport{ shaderEntryPoint };
        libExport.SetExportName(shaderEntryPoint + "_UID_" + std::to_string(mUniqueShaderExportID));
        ++mUniqueShaderExportID;
        return libExport;
    }

    void RayTracingPipelineState::AddExportToCollection(const LibraryExport& libraryExport, const Library* library)
    {
        if (library != mCurrentLibrary)
        {
            if (mLibraryExportCollections.find(library) == mLibraryExportCollections.end())
            {
                mLibraryExportCollections.emplace(library, library);
            }

            mCurrentLibrary = library;
            mCurrentExportsCollection = &mLibraryExportCollections.at(library);
        }

        mCurrentExportsCollection->AddExport(libraryExport);
    }

    void RayTracingPipelineState::GenerateLibraryExportCollectionsAndHitGroups()
    {
        assert_format(mRayGenerationShader.ShaderLibrary && mRayGenerationExport, "Ray Generation shader is missing");

        mLibraryExportCollections.clear();
        mCurrentExportsCollection = nullptr;
        mCurrentLibrary = nullptr;

        AddExportToCollection(*mRayGenerationExport, mRayGenerationShader.ShaderLibrary);

        for (auto i = 0; i < mMissShaders.size(); ++i)
        {
            AddExportToCollection(mMissShaderExports[i], mMissShaders[i].ShaderLibrary);
        }

        for (auto i = 0; i < mCallableShaders.size(); ++i)
        {
            AddExportToCollection(mCallableShaderExports[i], mCallableShaders[i].ShaderLibrary);
        }

        for (auto i = 0; i < mHitGroupShaders.size(); ++i)
        {
            const HitGroupShaders& hgShaders = mHitGroupShaders[i];
            const HitGroupLibraryExports& hgExports = mHitGroupExports[i];

            if (hgShaders.AnyHitLibrary) AddExportToCollection(*hgExports.AnyHitExport, hgShaders.AnyHitLibrary);
            if (hgShaders.ClosestHitLibrary) AddExportToCollection(*hgExports.ClosestHitExport, hgShaders.ClosestHitLibrary);
            if (hgShaders.IntersectionLibrary) AddExportToCollection(*hgExports.IntersectionExport, hgShaders.IntersectionLibrary);

            const LibraryExport* closestHitExport = hgExports.ClosestHitExport ? &hgExports.ClosestHitExport.value() : nullptr;
            const LibraryExport* anyHitExport = hgExports.AnyHitExport ? &hgExports.AnyHitExport.value() : nullptr;
            const LibraryExport* intersectionExport = hgExports.IntersectionExport ? &hgExports.IntersectionExport.value() : nullptr;

            mHitGroups.emplace_back(closestHitExport, anyHitExport, intersectionExport);
        }
    }

    void RayTracingPipelineState::AssociateExportsWithLocalRootSignaturesAndShaderConfig()
    {
        // Add local root signature and shader config associations
        AssociateConfigWithExport(mShaderConfig, *mRayGenerationExport);
        AssociateRootSignatureWithExport(mRayGenerationShader.LocalRootSignature, *mRayGenerationExport);

        for (auto i = 0; i < mMissShaders.size(); ++i)
        {
            const LibraryExport& missShaderExport = mMissShaderExports[i];
            AssociateConfigWithExport(mShaderConfig, missShaderExport);
            AssociateRootSignatureWithExport(mMissShaders[i].LocalRootSignature, missShaderExport);
        }

        for (auto i = 0; i < mCallableShaders.size(); ++i)
        {
            const LibraryExport& callableShaderExport = mCallableShaderExports[i];
            AssociateConfigWithExport(mShaderConfig, callableShaderExport);
            AssociateRootSignatureWithExport(mCallableShaders[i].LocalRootSignature, callableShaderExport);
        }

        for (auto i = 0; i < mHitGroupShaders.size(); ++i)
        {
            const HitGroupShaders& hgShaders = mHitGroupShaders[i];
            const HitGroupLibraryExports& hgExports = mHitGroupExports[i];

            if (hgShaders.AnyHitLibrary)
            {
                AssociateConfigWithExport(mShaderConfig, *hgExports.AnyHitExport);
                AssociateRootSignatureWithExport(hgShaders.LocalRootSignature, *hgExports.AnyHitExport);
            }

            if (hgShaders.ClosestHitLibrary)
            {
                AssociateConfigWithExport(mShaderConfig, *hgExports.ClosestHitExport);
                AssociateRootSignatureWithExport(hgShaders.LocalRootSignature, *hgExports.ClosestHitExport);
            }

            if (hgShaders.IntersectionLibrary)
            {
                AssociateConfigWithExport(mShaderConfig, *hgExports.IntersectionExport);
                AssociateRootSignatureWithExport(hgShaders.LocalRootSignature, *hgExports.IntersectionExport);
            }
        }
    }

    void RayTracingPipelineState::AddLibraryExportCollectionSubobjects()
    {
        // Add libraries
        for (auto& [library, exportsCollection] : mLibraryExportCollections)
        {
            AddLibraryExportsCollectionSubobject(exportsCollection);
        }
    }

    void RayTracingPipelineState::AddHitGroupSubobjects()
    {
        for (RayTracingHitGroupExport& hitGroup : mHitGroups)
        {
            AddHitGroupSubobject(hitGroup);
        }
    }

    void RayTracingPipelineState::AddLibraryExportsCollectionSubobject(const LibraryExportsCollection& exports)
    {
        const D3D12_DXIL_LIBRARY_DESC& d3dExportsCollection = exports.GetD3DLibraryExports();

        // Hold in memory until compilation
        D3D12_STATE_SUBOBJECT& librarySubobject = 
            mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &d3dExportsCollection });
    }

    void RayTracingPipelineState::AssociateConfigWithExport(const RayTracingShaderConfig& config, const LibraryExport& shaderExport)
    {
        mExportNamePointerHolder.emplace_back(shaderExport.ExportName().c_str());

        // Associate shader exports with shader config
        D3D12_STATE_SUBOBJECT& configSubobject = 
            mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &config.D3DConfig() });

        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& configToExportAssociation = 
            mAssociations.emplace_back(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION{ &configSubobject, 1, &mExportNamePointerHolder.back() });

        mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &configToExportAssociation });
    }

    void RayTracingPipelineState::AssociateRootSignatureWithExport(const RootSignature* signature, const LibraryExport& shaderExport)
    {
        if (!signature)
        {
            return;
        }

        mExportNamePointerHolder.emplace_back(shaderExport.ExportName().c_str());

        // Associate local root signatures with shader exports
        // Hold in memory until compilation
        D3D12_STATE_SUBOBJECT& localRootSignatureSubobject = 
            mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, signature->D3DSignature() });

        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& localRootSigToExportAssociation = 
            mAssociations.emplace_back(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION{ &localRootSignatureSubobject, 1, &mExportNamePointerHolder.back() });

        mSubobjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &localRootSigToExportAssociation });
    }

    void RayTracingPipelineState::AddHitGroupSubobject(const RayTracingHitGroupExport& group)
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

        mShaderTable.SetRayGenerationShader(GetShaderIdentifier(mRayGenerationExport->ExportName()), mRayGenerationShader.LocalRootSignature);

        for (auto i = 0; i < mMissShaders.size(); ++i)
        {
            mShaderTable.AddRayMissShader(GetShaderIdentifier(mMissShaderExports[i].ExportName()), mMissShaders[i].LocalRootSignature);
        }

        for (auto i = 0; i < mCallableShaders.size(); ++i)
        {
            mShaderTable.AddCallableShader(GetShaderIdentifier(mCallableShaderExports[i].ExportName()), mCallableShaders[i].LocalRootSignature);
        }

        for (auto i = 0; i < mHitGroupShaders.size(); ++i)
        {
            const HitGroupShaders& hgShaders = mHitGroupShaders[i];
            const RayTracingHitGroupExport& hitGroup = mHitGroups[i];

            mShaderTable.AddRayTracingHitGroupShaders(GetShaderIdentifier(hitGroup.ExportName()), hgShaders.LocalRootSignature);
        }
    }

}
