#pragma once

#include "GraphicAPIObject.hpp"
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
#include "DXILLibrary.hpp"
#include "RayTracingHitGroup.hpp"
#include "RayTracingPipelineConfig.hpp"
#include "RayTracingShaderConfig.hpp"
#include "ShaderTable.hpp"

#include <variant>
#include <unordered_map>

namespace HAL
{

    class PipelineState : public GraphicAPIObject
    {
    public:
        PipelineState(const Device* device);
        virtual ~PipelineState() = 0;

        virtual void ReplaceShader(const Shader* oldShader, const Shader* newShader) = 0;
        virtual void Compile() = 0;
        virtual void SetDebugName(const std::string& name) override;

    protected:
        Microsoft::WRL::ComPtr<ID3D12PipelineState> mState;
        const RootSignature* mRootSignature = nullptr;
        const Device* mDevice;
        std::string mDebugName;

    public:
        inline ID3D12PipelineState* D3DCompiledState() const { return mState.Get(); }
        inline const RootSignature* GetRootSignature() const { return mRootSignature; }

        inline void SetRootSignature(const RootSignature* signature) { mRootSignature = signature; }
    };

    class GraphicsPipelineState : public PipelineState
    {
    public:
        using RenderTargetFormat = std::variant<ResourceFormat::Color, ResourceFormat::TypelessColor>;
        using RenderTargetFormatMap = std::unordered_map<RenderTarget, RenderTargetFormat>;

        using PipelineState::PipelineState;

        ~GraphicsPipelineState() = default;

        void Compile() override;
        void ReplaceShader(const Shader* oldShader, const Shader* newShader) override;

        GraphicsPipelineState Clone() const;

        inline void SetVertexShader(const Shader* vertexShader) { mVertexShader = vertexShader; }
        inline void SetPixelShader(const Shader* pixelShader) { mPixelShader = pixelShader; }
        inline void SetDomainShader(const Shader* domainShader) { mDomainShader = domainShader; }
        inline void SetHullShader(const Shader* hullShader) { mHullShader = hullShader; }
        inline void SetGeometryShader(const Shader* geometryShader) { mGeometryShader = geometryShader; }
        inline void SetPrimitiveTopology(PrimitiveTopology topology) { mPrimitiveTopology = topology; }
        inline void SetDepthStencilFormat(ResourceFormat::DepthStencil format) { mDepthStencilFormat = format; }
        inline void SetInputAssemblerLayout(const InputAssemblerLayout& layout) { mInputLayout = layout; }
        inline void SetBlendState(const BlendState& state) { mBlendState = state; }
        inline void SetRasterizerState(const RasterizerState& state) { mRasterizerState = state; }
        inline void SetDepthStencilState(const DepthStencilState& state) { mDepthStencilState = state; }

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
            if (rt1) mRenderTargetFormats[RenderTarget::RT1] = *rt1;
            if (rt2) mRenderTargetFormats[RenderTarget::RT2] = *rt2;
            if (rt3) mRenderTargetFormats[RenderTarget::RT3] = *rt3;
            if (rt4) mRenderTargetFormats[RenderTarget::RT4] = *rt4;
            if (rt5) mRenderTargetFormats[RenderTarget::RT5] = *rt5;
            if (rt6) mRenderTargetFormats[RenderTarget::RT6] = *rt6;
            if (rt7) mRenderTargetFormats[RenderTarget::RT7] = *rt7;
        }

        inline void SetShaders(const GraphicsShaderBundle& bundle)
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

        inline const BlendState& GetBlendState() const { return mBlendState; }
        inline const RasterizerState& GetRasterizerState() const { return mRasterizerState; }
        inline const DepthStencilState& GetDepthStencilState() const { return mDepthStencilState; }
        inline const PrimitiveTopology& GetPrimitiveTopology() const { return mPrimitiveTopology; }

    private:
        const Shader* mVertexShader;
        const Shader* mPixelShader;
        const Shader* mDomainShader;
        const Shader* mHullShader;
        const Shader* mGeometryShader;
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
        using PipelineState::PipelineState;

        virtual void Compile() override;
        void ReplaceShader(const HAL::Shader* oldShader, const HAL::Shader* newShader) override;

        ComputePipelineState Clone() const;

        inline void SetShaders(const ComputeShaderBundle& bundle) { mComputeShader = bundle.ComputeShader(); }

    private:
        const Shader* mComputeShader;
    };



    class RayTracingPipelineState : public GraphicAPIObject
    {
    public:
        RayTracingPipelineState(const Device* device);

        void AddShaders(const RayTracingShaderBundle& bundle, const RayTracingShaderConfig& config, const RootSignature* localRootSignature = nullptr);
        void SetConfig(const RayTracingPipelineConfig& config);
        void SetGlobalRootSignature(const RootSignature* signature);
        void ReplaceShader(const Shader* oldShader, const Shader* newShader);
        void Compile();

        virtual void SetDebugName(const std::string& name) override;

    private:
        struct ConfiguredShaderPackage
        {
            const Shader* RayGenerationShader = nullptr;
            const Shader* ClosestHitShader = nullptr;
            const Shader* AnyHitShader = nullptr;
            const Shader* MissShader = nullptr;
            const Shader* IntersectionShader = nullptr;
            RayTracingShaderConfig Config;
            const RootSignature* LocalRootSignature = nullptr;
        };

        std::wstring GenerateUniqueExportName(const Shader& shader);
        void GenerateLibrariesAndHitGroups();
        DXILLibrary& GenerateLibrary(const Shader* shader, const RootSignature* localRootSignature);
        void AssociateLibraryWithItsExport(const DXILLibrary& library);
        void AssociateConfigWithExport(const RayTracingShaderConfig& config, const ShaderExport& shaderExport);
        void AssociateRootSignatureWithExport(const RootSignature& signature, const ShaderExport& shaderExport);
        void AddHitGroupSubobject(const RayTracingHitGroup& group);
        void AddGlobalRootSignatureSubobject();
        void AddPipelineConfigSubobject();
        void BuildShaderTable();

        const Device* mDevice = nullptr;

        const RootSignature* mGlobalRootSignature = nullptr;
        D3D12_GLOBAL_ROOT_SIGNATURE mD3DGlobalRootSignanture{};

        std::string mDebugName;
        uint32_t mUniqueShaderExportID = 0;
        RayTracingPipelineConfig mConfig;
        D3D12_STATE_OBJECT_DESC mRTPSODesc{};

        std::vector<ConfiguredShaderPackage> mShaders;

        std::vector<DXILLibrary> mLibraries;
        std::vector<RayTracingHitGroup> mHitGroups;

        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> mAssociations;
        std::vector<D3D12_STATE_SUBOBJECT> mSubobjects;
        std::vector<LPCWSTR> mExportNamePointerHolder;

        Microsoft::WRL::ComPtr<ID3D12StateObject> mState;
        Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> mProperties;

        ShaderTable mShaderTable;

    public:
        inline ID3D12StateObject* D3DCompiledState() const { return mState.Get(); }
        inline const RootSignature* GetGlobalRootSignature() const { return mGlobalRootSignature; }
    };

}

