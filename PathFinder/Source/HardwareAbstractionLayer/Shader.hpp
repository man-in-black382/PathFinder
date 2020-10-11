#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <array>
#include <unordered_map>
#include <filesystem>
#include <dxcapi.h>

#include "GraphicAPIObject.hpp"

#include <Foundation/Name.hpp>

namespace HAL
{
    
    struct ShaderIdentifier
    {
    public:
        ShaderIdentifier()
        {
            RawData.fill(0);
        }

        std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> RawData;
    };

    struct CompiledBinary
    {
        uint8_t* Data = nullptr;
        uint64_t Size = 0;
    };



    class Shader : public GraphicAPIObject
    {
    public:
        enum class Stage 
        {
            Vertex, Hull, Domain, Geometry, Pixel, Compute
        };

        Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const Microsoft::WRL::ComPtr<IDxcBlob>& pdbBlob, const std::string& entryPoint, Stage stage);

        void SetDebugName(const std::string& name) override;

    private:
        std::string mEntryPoint;
        Foundation::Name mEntryPointName;
        Stage mStage;
        std::string mDebugName;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> mPDBBlob;
        CompiledBinary mBinary;
        CompiledBinary mPDBBinary;

    public:
        inline const IDxcBlob* Blob() const { return mBlob.Get(); }
        inline const IDxcBlob* PDBBlob() const { return mPDBBlob.Get(); }
        inline const CompiledBinary& Binary() const { return mBinary; }
        inline const CompiledBinary& PDBBinary() const { return mPDBBinary; }
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }
        inline const std::string& EntryPoint() const { return mEntryPoint; }
        inline Foundation::Name EntryPointName() { return mEntryPointName; }
        inline const Stage PipelineStage() const { return mStage; }
        inline const std::string& DebugName() const { return mDebugName; }
    };



    class Library : public GraphicAPIObject
    {
    public:
        Library(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const Microsoft::WRL::ComPtr<IDxcBlob>& pdbBlob);

        void SetDebugName(const std::string& name) override;

    private:
        std::string mDebugName;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> mPDBBlob;
        CompiledBinary mBinary;
        CompiledBinary mPDBBinary;

    public:
        inline const IDxcBlob* Blob() const { return mBlob.Get(); }
        inline const IDxcBlob* PDBBlob() const { return mPDBBlob.Get(); }
        inline const CompiledBinary& Binary() const { return mBinary; }
        inline const CompiledBinary& PDBBinary() const { return mPDBBinary; }
        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }
        inline const std::string& DebugName() const { return mDebugName; }
    };

}

