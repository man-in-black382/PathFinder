#pragma once

#include <d3d12.h>
#include <memory>

#include "RootSignature.hpp"
#include "RingBufferResource.hpp"

namespace HAL
{

    class ShaderTable
    {
    public:
        using ShaderID = uint32_t;

        ShaderTable(const Device* device, uint8_t frameCapacity = 1);

        void AddShader(ShaderID id, const RootSignature* localRootSignature = nullptr);
        void UploadToGPUMemory();

    private:
        struct TableEntry
        {
            ShaderID ID;
            const RootSignature* Signature;
        };

        const Device* mDevice;
        uint8_t mFrameCapacity;
        std::unique_ptr<RingBufferResource<uint8_t>> mGPUTable;
        std::vector<TableEntry> mEntries;
    };

}

