#pragma once

#include "Device.hpp"
#include "RootSignature.hpp"
#include "Shader.hpp"
#include "RasterizerState.hpp"
#include "BlendState.hpp"
#include "DepthStencilState.hpp"
#include "PrimitiveTopology.hpp"
#include "ResourceFormat.hpp"
#include "InputAssemblerLayout.hpp"
#include "RenderTarget.hpp"

#include <variant>
#include <unordered_map>

namespace HAL
{

    class PipelineState {
    public:
        virtual ~PipelineState() = 0;

        virtual void Compile(const Device& device) = 0;

    protected:
        Microsoft::WRL::ComPtr<ID3D12PipelineState> mState;
        RootSignature* mRootSignature;

    public:
        inline ID3D12PipelineState* D3DCompiledState() const { return mState.Get(); }
        inline RootSignature* GetRootSignature() const { return mRootSignature; }

        inline void SetRootSignature(RootSignature* signature) { mRootSignature = signature; }
    };

    class GraphicsPipelineState : public PipelineState
    {
    public:
        using RenderTargetFormat = std::variant<ResourceFormat::Color, ResourceFormat::TypelessColor>;
        using RenderTargetFormatMap = std::unordered_map<RenderTarget, RenderTargetFormat>;

        ~GraphicsPipelineState() = default;

        virtual void Compile(const Device& device) override;

        GraphicsPipelineState Clone() const;

    private:
        Shader* mVertexShader;
        Shader* mPixelShader;
        Shader* mDomainShader;
        Shader* mHullShader;
        Shader* mGeometryShader;
        BlendState mBlendState;
        RasterizerState mRasterizerState;
        DepthStencilState mDepthStencilState;
        InputAssemblerLayout mInputLayout;
        RenderTargetFormatMap mRenderTargetFormats;
        ResourceFormat::DepthStencil mDepthStencilFormat;
        PrimitiveTopology mPrimitiveTopology;

    public:
        inline void SetRootSignature(RootSignature* rootSignature) { mRootSignature = rootSignature; }
        inline void SetVertexShader(Shader* vertexShader) { mVertexShader = vertexShader; }
        inline void SetPixelShader(Shader* pixelShader) { mPixelShader = pixelShader; }
        inline void SetDomainShader(Shader* domainShader) { mDomainShader = domainShader; }
        inline void SetHullShader(Shader* hullShader) { mHullShader = hullShader; }
        inline void SetGeometryShader(Shader* geometryShader) { mGeometryShader = geometryShader; }
        inline void SetPrimitiveTopology(PrimitiveTopology topology) { mPrimitiveTopology = topology; }
        inline void SetDepthStencilFormat(ResourceFormat::DepthStencil format) { mDepthStencilFormat = format; }

        inline BlendState& GetBlendState() { return mBlendState; }
        inline RasterizerState& GetRasterizerState() { return mRasterizerState; }
        inline DepthStencilState& GetDepthStencilState() { return mDepthStencilState; }
        inline InputAssemblerLayout& GetInputLayout() { return mInputLayout; }
        inline RenderTargetFormatMap& GetRenderTargetFormats() { return mRenderTargetFormats; }
        inline ResourceFormat::DepthStencil& GetDepthStencilFormat() { return mDepthStencilFormat; }
        inline PrimitiveTopology& GetPrimitiveTopology() { return mPrimitiveTopology; }
    };

    class ComputePipelineState {
    private:
        D3D12_COMPUTE_PIPELINE_STATE_DESC mDesc{};
    };

}

