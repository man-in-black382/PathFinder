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

    protected:
        PipelineState(const RootSignature& assisiatedRootSignature, PrimitiveTopology assosiatedTopology);

        Microsoft::WRL::ComPtr<ID3D12PipelineState> mState;
        RootSignature mAssosiatedRootSignature;
        PrimitiveTopology mAssosiatedTopology;

    public:
        inline const auto D3DState() const { return mState.Get(); }
        inline const auto& AssosiatedRootSignature() const { return mAssosiatedRootSignature; }
        inline const auto& AssosiatedPrimitiveTopology() const { return mAssosiatedTopology; }
    };

    class GraphicsPipelineState : public PipelineState
    {
    public:
        using RenderTargetFormat = std::variant<ResourceFormat::Color, ResourceFormat::TypelessColor>;
        using RenderTargetFormatMap = std::unordered_map<RenderTarget, RenderTargetFormat>;

        GraphicsPipelineState(
            const Device& device,
            const RootSignature& rootSignature,
            const Shader& vertexShader,
            const Shader& pixelShader,
            const Shader* domainShader,
            const Shader* hullShader,
            const Shader* geometryShader,
            const BlendState& blendState,
            const RasterizerState& rasterizerState,
            const DepthStencilState& depthStencilState,
            const InputAssemblerLayout& inputLayout,
            const RenderTargetFormatMap& renderTargetFormats,
            ResourceFormat::DepthStencil depthStencilFormat,
            PrimitiveTopology primitiveTopology
        );

        ~GraphicsPipelineState() = default;
    };

    class ComputePipelineState {
    private:
        D3D12_COMPUTE_PIPELINE_STATE_DESC mDesc{};
    };

}

