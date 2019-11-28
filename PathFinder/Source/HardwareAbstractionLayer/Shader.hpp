#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <dxcapi.h>

#include "GraphicAPIObject.hpp"

namespace HAL
{
    
    class Shader : public GraphicAPIObject
    {
    public:
        enum class Stage 
        {
            Vertex, Hull, Domain, Geometry, Pixel, Compute,
            RayGeneration, RayClosestHit, RayAnyHit, RayMiss, RayIntersection
        };

        enum class Profile { P5_1, P6_3 };

        Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::wstring& entryPoint, Stage stage);
        
    private:
        std::wstring mEntryPoint;
        Stage mStage;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;

    public:
        inline const IDxcBlob* Blob() const { return mBlob.Get(); }
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }
        inline const std::wstring& EntryPoint() const { return mEntryPoint; }
        inline const Stage PipelineStage() const { return mStage; }
    };



    class GraphicsShaderBundle
    {
    public:
        GraphicsShaderBundle(Shader* vs, Shader* ps, Shader* ds, Shader* hs, Shader* gs);

    private:
        const Shader* mVertexShader = nullptr;
        const Shader* mPixelShader = nullptr;
        const Shader* mDomainShader = nullptr;
        const Shader* mHullShader = nullptr;
        const Shader* mGeometryShader = nullptr;

    public:
        inline const Shader* VertexShader() const { return mVertexShader; }
        inline const Shader* PixelShader() const { return mPixelShader; }
        inline const Shader* DomainShader() const { return mDomainShader; }
        inline const Shader* HullShader() const { return mHullShader; }
        inline const Shader* GeometryShader() const { return mGeometryShader; }
    };



    class ComputeShaderBundle
    {
    public:
        ComputeShaderBundle(Shader* cs);

    private:
        const Shader* mComputeShader = nullptr;

    public:
        inline const Shader* ComputeShader() const { return mComputeShader; }
    };



    class RayTracingShaderBundle
    {
    public:
        RayTracingShaderBundle(Shader* rayGeneration, Shader* closestHit, Shader* anyHit, Shader* miss, Shader* intersection = nullptr);
        
    private:
        const Shader* mRayGenerationShader = nullptr;
        const Shader* mClosestHitShader = nullptr;
        const Shader* mAnyHitShader = nullptr;
        const Shader* mMissShader = nullptr;
        const Shader* mIntersectionShader = nullptr;

    public:
        inline const Shader* RayGenerationShader() const { return mRayGenerationShader; }
        inline const Shader* ClosestHitShader() const { return mClosestHitShader; }
        inline const Shader* AnyHitShader() const { return mAnyHitShader; }
        inline const Shader* MissShader() const { return mMissShader; }
        inline const Shader* IntersectionShader() const { return mIntersectionShader; }
    };

}

