#include "PipelineState.hpp"
#include "Utils.h"

#include "../Foundation/STDHelpers.hpp"

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



    void RayTracingPipelineState::AddShaderBundle(const RayTracingShaderBundle& bundle, const RootSignature* localRootSignature)
    {
        ShaderExport* closestHitExport = nullptr;
        ShaderExport* anyHitExport = nullptr;
        ShaderExport* intersectionExport = nullptr;

        AddDXILLibrary(bundle.RayGenerationShader(), localRootSignature);
        AddDXILLibrary(bundle.MissShader(), localRootSignature);

        closestHitExport = &AddDXILLibrary(bundle.ClosestHitShader(), localRootSignature);
        anyHitExport = &AddDXILLibrary(bundle.AnyHitShader(), localRootSignature);

        if (bundle->IntersectionShader()) 
        {
            intersectionExport = &AddDXILLibrary(bundle.IntersectionShader(), localRootSignature);
        }

        mHitGroups.emplace_back(closestHitExport, anyHitExport, intersectionExport);
    }

    void RayTracingPipelineState::SetGlobalRootSignature(const RootSignature* signature)
    {
        mGlobalRootSignature = signature;

        mDevice->D3DDevice()->CreateStateObject()
    }

    void RayTracingPipelineState::Compile()
    {
        // Build associations 
        for (auto& pair : mDXILLibraries)
        {
            DXILLibrary& library = pair->second;
            ShaderExport& shaderExport = library.Export();

            D3D12_STATE_SUBOBJECT librarySubobject{ D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &library.D3DLibrary() };
            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association{ &librarySubobject, 1, shaderExport.ExportName().c_str() };
            D3D12_STATE_SUBOBJECT associationSubobject{ D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &association };
        }
        /*typedef
            enum D3D12_STATE_SUBOBJECT_TYPE
        {
            D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG = 0,
            D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE = 1,
            D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE = 2,
            D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK = 3,
            D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY = 5,
            D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION = 6,
            D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION = 7,
            D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION = 8,
            D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG = 9,
            D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG = 10,
            D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP = 11,
         */
        mDevice->D3DDevice()->CreateStateObject(&mRTPSODesc, IID_PPV_ARGS(mState.GetAddressOf()));
    }

    std::wstring RayTracingPipelineState::GenerateUniqueExportName(const Shader& shader)
    {
        mUniqueShaderID++;
        return std::to_wstring(mUniqueShaderID) + shader.EntryPoint();
    }

    HAL::DXILLibrary& RayTracingPipelineState::AddDXILLibrary(const Shader* shader, const RootSignature* localRootSignature)
    {
        ShaderExport shaderExport{ shader };
        shaderExport.SetExportName(GenerateUniqueExportName(*shader));
        DXILLibrary lib{ shaderExport };
        lib.SetLocalRootSignature(localRootSignature);
        return mDXILLibraries.emplace(shader, lib).second;



    /*    D3D12_STATE_SUBOBJECT dxilLibrarySubobject{};
        dxilLibrarySubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        dxilLibrarySubobject.pDesc = &shader.D3DDXILLibrary();
        mSubobjects.push_back(dxilLibrarySubobject);*/
    }

}
