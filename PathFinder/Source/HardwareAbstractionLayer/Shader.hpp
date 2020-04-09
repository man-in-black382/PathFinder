#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <array>
#include <unordered_map>
#include <filesystem>
#include <dxcapi.h>

#include "GraphicAPIObject.hpp"

#include "../Foundation/Name.hpp"

namespace HAL
{
    
    struct ShaderIdentifier
    {
    public:
        ShaderIdentifier()
        {
            RawData.fill(255);
        }

        std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> RawData;
    };

    class Shader : public GraphicAPIObject
    {
    public:
        enum class Stage 
        {
            Vertex, Hull, Domain, Geometry, Pixel, Compute,
            RayGeneration, RayClosestHit, RayAnyHit, RayMiss, RayIntersection
        };

        enum class Profile { P6_3, P6_4 };

        Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::wstring& entryPoint, Stage stage);
        
    private:
        std::wstring mEntryPoint;
        Foundation::Name mEntryPointName;
        Stage mStage;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;

    public:
        inline const IDxcBlob* Blob() const { return mBlob.Get(); }
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }
        inline const std::wstring& EntryPoint() const { return mEntryPoint; }
        inline Foundation::Name EntryPointName() { return mEntryPointName; }
        inline const Stage PipelineStage() const { return mStage; }
    };

}

