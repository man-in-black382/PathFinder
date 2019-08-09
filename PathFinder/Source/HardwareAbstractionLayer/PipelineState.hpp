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

        inline void SetRootSignature(RootSignature* rootSignature) { mRootSignature = rootSignature; }
        inline void SetVertexShader(Shader* vertexShader) { mVertexShader = vertexShader; }
        inline void SetPixelShader(Shader* pixelShader) { mPixelShader = pixelShader; }
        inline void SetDomainShader(Shader* domainShader) { mDomainShader = domainShader; }
        inline void SetHullShader(Shader* hullShader) { mHullShader = hullShader; }
        inline void SetGeometryShader(Shader* geometryShader) { mGeometryShader = geometryShader; }
        inline void SetPrimitiveTopology(PrimitiveTopology topology) { mPrimitiveTopology = topology; }
        inline void SetDepthStencilFormat(ResourceFormat::DepthStencil format) { mDepthStencilFormat = format; }
        inline void SetInputAssemblerLayout(const InputAssemblerLayout& layout) { mInputLayout = layout; }

        inline void SetRenderTargetFormats(
            std::optional<RenderTargetFormat> rt0 = std::nullopt,
            std::optional<RenderTargetFormat> rt1 = std::nullopt,
            std::optional<RenderTargetFormat> rt2 = std::nullopt,
            std::optional<RenderTargetFormat> rt3 = std::nullopt,
            std::optional<RenderTargetFormat> rt4 = std::nullopt,
            std::optional<RenderTargetFormat> rt5 = std::nullopt,
            std::optional<RenderTargetFormat> rt6 = std::nullopt,
            std::optional<RenderTargetFormat> rt7 = std::nullopt)
        {
            if (rt0) mRenderTargetFormats[RenderTarget::RT0] = *rt0;
            if (rt1) mRenderTargetFormats[RenderTarget::RT0] = *rt1;
            if (rt2) mRenderTargetFormats[RenderTarget::RT0] = *rt2;
            if (rt3) mRenderTargetFormats[RenderTarget::RT0] = *rt3;
            if (rt4) mRenderTargetFormats[RenderTarget::RT0] = *rt4;
            if (rt5) mRenderTargetFormats[RenderTarget::RT0] = *rt5;
            if (rt6) mRenderTargetFormats[RenderTarget::RT0] = *rt6;
            if (rt7) mRenderTargetFormats[RenderTarget::RT0] = *rt7;
        }

        inline void SetShaders(const ShaderBundle& bundle)
        {
            mVertexShader = bundle.VertexShader();
            mPixelShader = bundle.PixelShader();
            mDomainShader = bundle.DomainShader();
            mHullShader = bundle.HullShader();
            mGeometryShader = bundle.GeometryShader();
        }

        inline BlendState& GetBlendState() { return mBlendState; }
        inline RasterizerState& GetRasterizerState() { return mRasterizerState; }
        inline DepthStencilState& GetDepthStencilState() { return mDepthStencilState; }

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
    };



    class ComputePipelineState : public PipelineState {
    public:
        virtual void Compile(const Device& device) override;

        ComputePipelineState Clone() const;

        inline void SetShaders(const ShaderBundle& bundle) { mComputeShader = bundle.ComputeShader(); }
        inline void SetComputeShader(Shader* computeShader) { mComputeShader = computeShader; }

    private:
        Shader* mComputeShader;
    };

}

