#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace HAL
{
    
    class Shader
    {
    public:
        enum class PipelineStage { Vertex, Hull, Domain, Geometry, Pixel, Compute };

        Shader(const std::wstring& filePath, PipelineStage pipelineStage);
        Shader(const std::string& filePath, PipelineStage pipelineStage);

    private:
        Microsoft::WRL::ComPtr<ID3DBlob> mBlob;
        void* mBytecode;
        uint64_t mBytecodeSize;

    public:
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBytecode, mBytecodeSize }; }
    };



    class ShaderBundle
    {
    public:
        ShaderBundle(Shader* vs, Shader* ps, Shader* ds, Shader* hs, Shader* gs, Shader* cs);

    private:
        Shader* mVertexShader;
        Shader* mPixelShader;
        Shader* mDomainShader;
        Shader* mHullShader;
        Shader* mGeometryShader;
        Shader* mComputeShader;

    public:
        inline Shader* VertexShader() const { return mVertexShader; }
        inline Shader* PixelShader() const { return mPixelShader; }
        inline Shader* DomainShader() const { return mDomainShader; }
        inline Shader* HullShader() const { return mHullShader; }
        inline Shader* GeometryShader() const { return mGeometryShader; }
        inline Shader* ComputeShader() const { return mComputeShader; }
    };

}

