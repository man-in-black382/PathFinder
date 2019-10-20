#include "PipelineState.hpp"
#include "Utils.h"

#include "../Foundation/STDHelpers.hpp"
#include "../Foundation/StringUtils.hpp"

#include <d3d12.h>

namespace HAL
{

    PipelineState::PipelineState(const Device* device)
        : mDevice{ device } {}

    PipelineState::~PipelineState() {}

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

        //desc.SampleDesc
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

    void PipelineState::SetDebugName(const std::string& name)
    {
        if (mState)
        {
            mState->SetName(StringToWString(name).c_str());
        }
        
        mDebugName = name;
    }



    GraphicsPipelineState GraphicsPipelineState::Clone() const
    {
        GraphicsPipelineState newState = *this;
        newState.mState = nullptr;
        return newState;
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
    }

    ComputePipelineState ComputePipelineState::Clone() const
    {
        ComputePipelineState newState = *this;
        newState.mState = nullptr;
        return newState;
    }



    RayTracingPipelineState::RayTracingPipelineState(const Device* device)
        : mDevice{ device }, mShaderTable{ device } {}

    void RayTracingPipelineState::AddShaders(const RayTracingShaderBundle& bundle, const RayTracingShaderConfig& config, const RootSignature* localRootSignature)
    {
        AddShader(bundle.RayGenerationShader(), config, localRootSignature);
        AddShader(bundle.MissShader(), config, localRootSignature);

        const ShaderExport* closestHitExport = &AddShader(bundle.ClosestHitShader(), config, localRootSignature).Library.Export();
        const ShaderExport* anyHitExport = &AddShader(bundle.AnyHitShader(), config, localRootSignature).Library.Export();
        const ShaderExport* intersectionExport = bundle.IntersectionShader() ?
            &AddShader(bundle.IntersectionShader(), config, localRootSignature).Library.Export() :
            nullptr;

        mHitGroups.emplace_back(closestHitExport, anyHitExport, intersectionExport);
    }

    void RayTracingPipelineState::SetConfig(const RayTracingPipelineConfig& config)
    {
        mConfig = config;
    }

    void RayTracingPipelineState::SetGlobalRootSignature(const RootSignature* signature)
    {
        mGlobalRootSignature = signature;
    }

    void RayTracingPipelineState::Compile()
    {
        for (auto& pair : mShaderAssociations)
        {
            const ShaderAssociations& shaderAssociations = pair.second;
            const DXILLibrary& library = shaderAssociations.Library;
            const ShaderExport& shaderExport = library.Export();
            const RayTracingShaderConfig& shaderConfig = shaderAssociations.Config;

            AssociateLibraryWithItsExport(library);
            AssociateConfigWithExport(shaderConfig, shaderExport);

            if (library.LocalRootSignature())
            {
                AssociateRootSignatureWithExport(*library.LocalRootSignature(), shaderExport);
            }
        }

        for (RayTracingHitGroup& hitGroup : mHitGroups)
        {
            AddHitGroupSubobject(hitGroup);
        }

        AddGlobalRootSignatureSubobject();
        AddPipelineConfigSubobject();

        mRTPSODesc.NumSubobjects = (UINT)mSubobjects.size();
        mRTPSODesc.pSubobjects = mSubobjects.data();

        mDevice->D3DDevice()->CreateStateObject(&mRTPSODesc, IID_PPV_ARGS(mState.GetAddressOf()));
        mState->QueryInterface(IID_PPV_ARGS(mProperties.GetAddressOf()));

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

    std::wstring RayTracingPipelineState::GenerateUniqueExportName(const Shader& shader)
    {
        mUniqueShaderExportID++;
        return std::to_wstring(mUniqueShaderExportID) + shader.EntryPoint();
    }

    RayTracingPipelineState::ShaderAssociations& RayTracingPipelineState::AddShader(const Shader* shader, const RayTracingShaderConfig& config, const RootSignature* localRootSignature)
    {
        ShaderExport shaderExport{ shader };
        shaderExport.SetExportName(GenerateUniqueExportName(*shader));
        DXILLibrary lib{ shaderExport };
        lib.SetLocalRootSignature(localRootSignature);
        ShaderAssociations associations{ lib, config };
        mShaderAssociations.emplace(shader, associations);
        return mShaderAssociations.at(shader);
    }

    void RayTracingPipelineState::AssociateLibraryWithItsExport(const DXILLibrary& library)
    {
        LPCWSTR exportName = library.Export().ExportName().c_str();

        D3D12_STATE_SUBOBJECT librarySubobject{ D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &library.D3DLibrary() };
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION libToExportAssociation{ &librarySubobject, 1, &exportName };
        D3D12_STATE_SUBOBJECT libToExportAssociationSubobject{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &libToExportAssociation };

        // Hold in memory until compilation
        mSubobjects.push_back(librarySubobject);
        mSubobjects.push_back(libToExportAssociationSubobject);
        mAssociations.push_back(libToExportAssociation);
    }

    void RayTracingPipelineState::AssociateConfigWithExport(const RayTracingShaderConfig& config, const ShaderExport& shaderExport)
    {
        LPCWSTR exportName = shaderExport.ExportName().c_str();

        // Associate shader exports with shader config
        D3D12_STATE_SUBOBJECT configSubobject{ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &config.D3DConfig() };
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION configToExportAssociation{ &configSubobject, 1, &exportName };
        D3D12_STATE_SUBOBJECT configToExportAssociationSubobject{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &configToExportAssociation };

        mSubobjects.push_back(configSubobject);
        mSubobjects.push_back(configToExportAssociationSubobject);
        mAssociations.push_back(configToExportAssociation);
    }

    void RayTracingPipelineState::AssociateRootSignatureWithExport(const RootSignature& signature, const ShaderExport& shaderExport)
    {
        LPCWSTR exportName = shaderExport.ExportName().c_str();

        // Associate local root signatures with shader exports
        D3D12_STATE_SUBOBJECT localRootSignatureSubobject{ D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, signature.D3DSignature() };
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigToExportAssociation{ &localRootSignatureSubobject, 1, &exportName };
        D3D12_STATE_SUBOBJECT localRootSigToExportAssociationSubobject{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &localRootSigToExportAssociation };

        // Hold in memory until compilation
        mSubobjects.push_back(localRootSignatureSubobject);
        mSubobjects.push_back(localRootSigToExportAssociationSubobject);
        mAssociations.push_back(localRootSigToExportAssociation);
    }

    void RayTracingPipelineState::AddHitGroupSubobject(const RayTracingHitGroup& group)
    {
        D3D12_STATE_SUBOBJECT hitGroupSubobject{ D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &group.D3DHitGroup() };
        mSubobjects.push_back(hitGroupSubobject);
    }

    void RayTracingPipelineState::AddGlobalRootSignatureSubobject()
    {
        if (!mGlobalRootSignature) return;

        D3D12_STATE_SUBOBJECT globalRootSignatureSubobject{ D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, mGlobalRootSignature->D3DSignature() };
        mSubobjects.push_back(globalRootSignatureSubobject);
    }

    void RayTracingPipelineState::AddPipelineConfigSubobject()
    {
        D3D12_STATE_SUBOBJECT configSubobject{ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &mConfig.D3DConfig() };
        mSubobjects.push_back(configSubobject);
    }

    void RayTracingPipelineState::ClearInternalContainers()
    {
        mShaderAssociations.clear();
        mHitGroups.clear();
    }

    void RayTracingPipelineState::BuildShaderTable()
    {
        for (auto& pair : mShaderAssociations)
        {
            const ShaderAssociations& shaderAssociations = pair.second;
            const DXILLibrary& library = shaderAssociations.Library;
            const ShaderExport& shaderExport = library.Export();

            auto shaderId = reinterpret_cast<ShaderTable::ShaderID *>(mProperties->GetShaderIdentifier(shaderExport.ExportName().c_str()));
            mShaderTable.AddShader(*shaderExport.AssosiatedShader(), *shaderId, library.LocalRootSignature());
        }

        mShaderTable.UploadToGPUMemory();
    }

}
