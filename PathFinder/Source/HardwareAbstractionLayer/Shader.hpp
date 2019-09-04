#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <filesystem>

#include <dxcapi.h>

namespace HAL
{
    
    class Shader
    {
    public:
        enum class Stage 
        {
            Vertex, Hull, Domain, Geometry, Pixel, Compute,
            RayGeneration, RayHit, RayAnyHit, RayMiss, RayIntersection
        };

        enum class Profile { P5_1, P6_1 };

        Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::wstring& entryPoint, Stage stage);

    private:
        std::wstring mEntryPoint;
        Stage mStage;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;
        D3D12_EXPORT_DESC mExport{};
        D3D12_DXIL_LIBRARY_DESC mDXILLibrary{};

    public:
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }
        inline const std::wstring& EntryPoint() const { return mEntryPoint; }
        inline const D3D12_EXPORT_DESC& D3DExport() const { mExport; }
        inline const D3D12_DXIL_LIBRARY_DESC& D3DDXILLibrary() const { mDXILLibrary; }
    };



    class ShaderBundle
    {
    public:
        ShaderBundle(Shader* vs, Shader* ps, Shader* ds, Shader* hs, Shader* gs, Shader* cs);

    private:
        Shader* mVertexShader = nullptr;
        Shader* mPixelShader = nullptr;
        Shader* mDomainShader = nullptr;
        Shader* mHullShader = nullptr;
        Shader* mGeometryShader = nullptr;
        Shader* mComputeShader = nullptr;

    public:
        inline Shader* VertexShader() const { return mVertexShader; }
        inline Shader* PixelShader() const { return mPixelShader; }
        inline Shader* DomainShader() const { return mDomainShader; }
        inline Shader* HullShader() const { return mHullShader; }
        inline Shader* GeometryShader() const { return mGeometryShader; }
        inline Shader* ComputeShader() const { return mComputeShader; }
    };



    class RayTracingShaderBundle
    {
    public:
        RayTracingShaderBundle(Shader* rayGeneration, Shader* closestHit, Shader* anyHit, Shader* miss, Shader* intersection = nullptr);
        
    private:
        Shader* mRayGenerationShader = nullptr;
        Shader* mClosestHitShader = nullptr;
        Shader* mAnyHitShader = nullptr;
        Shader* mMissShader = nullptr;
        Shader* mIntersectionShader = nullptr;

    public:
        inline Shader* RayGenerationShader() const { return mRayGenerationShader; }
        inline Shader* ClosestHitShader() const { return mClosestHitShader; }
        inline Shader* AnyHitShader() const { return mAnyHitShader; }
        inline Shader* MissShader() const { return mMissShader; }
        inline Shader* IntersectionShader() const { return mIntersectionShader; }
    };

}

