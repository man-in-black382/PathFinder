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
#include "LibraryExportsCollection.hpp"
#include "RayTracingHitGroupExport.hpp"
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
        using RenderTargetFormat = std::variant<ColorFormat, TypelessColorFormat>;
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
        inline void SetDepthStencilFormat(DepthStencilFormat format) { mDepthStencilFormat = format; }
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
        DepthStencilFormat mDepthStencilFormat;
        PrimitiveTopology mPrimitiveTopology;
    };



    class ComputePipelineState : public PipelineState {
    public:
        using PipelineState::PipelineState;

        virtual void Compile() override;
        void ReplaceShader(const HAL::Shader* oldShader, const HAL::Shader* newShader) override;

        inline void SetComputeShader(const Shader* computeShader) { mComputeShader = computeShader; }

        ComputePipelineState Clone() const;

    private:
        const Shader* mComputeShader;
    };



    class RayTracingPipelineState : public GraphicAPIObject
    {
    public:
        struct SingleRTShader
        {
            const Library* ShaderLibrary = nullptr;
            std::string EntryPoint;
            const RootSignature* LocalRootSignature = nullptr;
        };

        struct HitGroupShaders
        {
            // Shader libraries containing RT shaders,
            // could be the same library for all of them
            const Library* ClosestHitLibrary = nullptr;
            const Library* AnyHitLibrary = nullptr;
            const Library* IntersectionLibrary = nullptr;

            // Entry point names required to extract individual shaders
            // from shader libraries using so-called library exports
            std::string ClosestHitEntryPoint;
            std::string AnyHitEntryPoint;
            std::string IntersectionEntryPoint;

            const RootSignature* LocalRootSignature = nullptr;
        };

        RayTracingPipelineState(const Device* device);

        void SetRayGenerationShader(const SingleRTShader& shader);
        void AddMissShader(const SingleRTShader& shader);
        void AddCallableShader(const SingleRTShader& shader);
        void AddHitGroupShaders(const HitGroupShaders& hgShaders);
        void SetPipelineConfig(const RayTracingPipelineConfig& config);
        void SetShaderConfig(const RayTracingShaderConfig& config);
        void SetGlobalRootSignature(const RootSignature* signature);
        void ReplaceLibrary(const Library* oldLibrary, const Library* newLibrary);
        void Compile();

        virtual void SetDebugName(const std::string& name) override;

    private:
        struct HitGroupLibraryExports
        {
            std::optional<LibraryExport> AnyHitExport;
            std::optional<LibraryExport> ClosestHitExport;
            std::optional<LibraryExport> IntersectionExport;
        };

        ShaderIdentifier GetShaderIdentifier(const std::wstring& exportName);
        LibraryExport GenerateLibraryExport(const std::string& shaderIntryPoint);
        void AddExportToCollection(const LibraryExport& libraryExport, const Library* library);
        void GenerateLibraryExportCollectionsAndHitGroups();

        void AssociateExportsWithLocalRootSignaturesAndShaderConfig();
        void AddLibraryExportCollectionSubobjects();
        void AddHitGroupSubobjects();

        void AddLibraryExportsCollectionSubobject(const LibraryExportsCollection& library);
        void AssociateConfigWithExport(const RayTracingShaderConfig& config, const LibraryExport& shaderExport);
        void AssociateRootSignatureWithExport(const RootSignature* signature, const LibraryExport& shaderExport);
        void AddHitGroupSubobject(const RayTracingHitGroupExport& group);
        void AddGlobalRootSignatureSubobject();
        void AddPipelineConfigSubobject();
        void BuildShaderTable();

        const Device* mDevice = nullptr;

        const RootSignature* mGlobalRootSignature = nullptr;
        D3D12_GLOBAL_ROOT_SIGNATURE mD3DGlobalRootSignanture{};

        std::string mDebugName;
        uint32_t mUniqueShaderExportID = 0;
        RayTracingPipelineConfig mPipelineConfig;
        RayTracingShaderConfig mShaderConfig;
        D3D12_STATE_OBJECT_DESC mRTPSODesc{};

        // Shader libraries, entry points and exports waiting for compilation
        SingleRTShader mRayGenerationShader;
        std::optional<LibraryExport> mRayGenerationExport;

        std::vector<SingleRTShader> mMissShaders;
        std::vector<LibraryExport> mMissShaderExports;

        std::vector<SingleRTShader> mCallableShaders;
        std::vector<LibraryExport> mCallableShaderExports;

        std::vector<HitGroupShaders> mHitGroupShaders;
        std::vector<HitGroupLibraryExports> mHitGroupExports;

        // Containers used during and after compilation
        std::unordered_map<const Library*, LibraryExportsCollection> mLibraryExportCollections;
        std::vector<RayTracingHitGroupExport> mHitGroups;
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> mAssociations;
        std::vector<D3D12_STATE_SUBOBJECT> mSubobjects;
        std::vector<LPCWSTR> mExportNamePointerHolder;

        // To save ourselves from redundant map accesses when a lot of shaders are added to PSO
        const Library* mCurrentLibrary = nullptr;
        LibraryExportsCollection* mCurrentExportsCollection = nullptr;

        Microsoft::WRL::ComPtr<ID3D12StateObject> mState;
        Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> mProperties;

        ShaderTable mShaderTable;

    public:
        inline ID3D12StateObject* D3DCompiledState() const { return mState.Get(); }
        inline const RootSignature* GetGlobalRootSignature() const { return mGlobalRootSignature; }
        inline ShaderTable& GetShaderTable() { return mShaderTable; }
    };

}

