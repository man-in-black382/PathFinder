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
            Vertex, Hull, Domain, Geometry, Pixel, Compute
        };

        Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const std::string& entryPoint, Stage stage);
        
    private:
        std::string mEntryPoint;
        Foundation::Name mEntryPointName;
        Stage mStage;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;

    public:
        inline const IDxcBlob* Blob() const { return mBlob.Get(); }
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }
        inline const std::string& EntryPoint() const { return mEntryPoint; }
        inline Foundation::Name EntryPointName() { return mEntryPointName; }
        inline const Stage PipelineStage() const { return mStage; }
    };



    class Library : public GraphicAPIObject
    {
    public:
        Library(const Microsoft::WRL::ComPtr<IDxcBlob>& blob);

    private:
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;

    public:
        inline const IDxcBlob* Blob() const { return mBlob.Get(); }
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }
    };

}

