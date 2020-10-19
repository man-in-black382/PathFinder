#pragma once

#include "GraphicAPIObject.hpp"
#include "Buffer.hpp"



#include <cstdint>
#include <d3d12.h>
#include <vector>

#include <glm/mat4x4.hpp>

namespace HAL
{
    
    struct RayTracingGeometry
    {
        RayTracingGeometry(const Buffer* vertexBuffer, uint32_t vertexOffset, uint32_t vertexCount, uint32_t vertexStride, ColorFormat vertexPositionFormat,
            const Buffer* indexBuffer, uint32_t indexOffset, uint32_t indexCount, uint32_t indexStride, ColorFormat indexFormat, const glm::mat4x4& transform,
            bool isOpaque = true)
            :
            VertexBuffer{ vertexBuffer }, IndexBuffer{ indexBuffer }, VertexOffset{ vertexOffset }, VertexCount{ vertexCount }, VertexStride{ vertexStride },
            IndexOffset{ indexOffset }, IndexCount{ indexCount }, IndexStride{ indexStride }, VertexPositionFormat{ vertexPositionFormat }, IndexFormat{ indexFormat },
            Transform{ transform }, IsOpaque{ isOpaque } {}

        // A format for 'Position' vertex structure field
        // Implies that 'Position' must be placed first in vertex's memory
        ColorFormat VertexPositionFormat;
        ColorFormat IndexFormat;
        const Buffer* VertexBuffer;
        const Buffer* IndexBuffer;
        uint32_t VertexOffset;
        uint32_t VertexCount;
        uint32_t VertexStride;
        uint32_t IndexOffset;
        uint32_t IndexCount;
        uint32_t IndexStride;
        glm::mat4x4 Transform;
        bool IsOpaque;
    };



    class RayTracingAccelerationStructure : public GraphicAPIObject
    {
    public:
        RayTracingAccelerationStructure(const Device* device);

        virtual void Clear() = 0;
        virtual void SetBuffers(const Buffer* destinationBuffer, const Buffer* scratchBuffer, const Buffer* updateBuffer = nullptr);

    protected:
        struct CommonMemoryRequirements
        {
            uint64_t DestinationBufferMaxSizeInBytes;
            uint64_t BuildScratchBufferSizeInBytes;
            uint64_t UpdateScratchBufferSizeInBytes;
        };

        CommonMemoryRequirements QueryCommonMemoryRequirements() const;

        const Device* mDevice;
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS mD3DInputs{};
    
    private:
        std::string mDebugName;
        const Buffer* mBuildScratchBuffer = nullptr;
        const Buffer* mFinalBuffer = nullptr;
        const Buffer* mUpdateBuffer = nullptr;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC mD3DAccelerationStructure{};

    public:
        inline const auto& D3DAccelerationStructure() const { return mD3DAccelerationStructure; }
        inline const auto* FinalBuffer() const { return mFinalBuffer; }
    };



    class RayTracingBottomAccelerationStructure : public RayTracingAccelerationStructure
    {
    public:
        struct MemoryRequirements
        {
            uint64_t DestinationBufferMaxSizeInBytes;
            uint64_t BuildScratchBufferSizeInBytes;
            uint64_t UpdateScratchBufferSizeInBytes;
        };

        using RayTracingAccelerationStructure::RayTracingAccelerationStructure;

        void AddGeometry(const RayTracingGeometry& geometry);
        MemoryRequirements QueryMemoryRequirements() const;

        virtual void Clear() override;

    private:
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> mD3DGeometries;
    };



    class RayTracingTopAccelerationStructure : public RayTracingAccelerationStructure
    {
    public:
        struct InstanceInfo
        {
            InstanceInfo(uint32_t id) : InstanceID{ id }, InstanceMask{ 0xFF } {}
            InstanceInfo(uint32_t id, uint32_t mask) : InstanceID{ id }, InstanceMask{ mask } {}

            uint32_t InstanceID : 24;
            uint32_t InstanceMask : 8;
        };

        struct MemoryRequirements
        {
            uint64_t DestinationBufferMaxSizeInBytes;
            uint64_t BuildScratchBufferSizeInBytes;
            uint64_t UpdateScratchBufferSizeInBytes;
            uint64_t InstanceBufferSizeInBytes;
        };

        using RayTracingAccelerationStructure::RayTracingAccelerationStructure;

        void AddInstance(const RayTracingBottomAccelerationStructure& blas, const InstanceInfo& instanceInfo, const glm::mat4& transform);
        MemoryRequirements QueryMemoryRequirements() const;

        void SetBuffers(
            uint8_t* instanceDataUploadPtr,
            const Buffer* instanceBuffer, 
            const Buffer* destinationBuffer, 
            const Buffer* scratchBuffer, 
            const Buffer* updateBuffer = nullptr
        );

        virtual void Clear() override;

    private:
        const Buffer* mInstanceBuffer = nullptr;
        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> mD3DInstances;
    };

}
