#pragma once

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include "../Memory/GPUResourceProducer.hpp"

#include "../Scene/Scene.hpp"
#include "../Scene/Mesh.hpp"
#include "../Scene/MeshInstance.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV.hpp"
#include "../Scene/Vertices/Vertex1P3.hpp"
#include "../Scene/FlatLight.hpp"
#include "../Scene/SphericalLight.hpp"

#include "VertexStorageLocation.hpp"
#include "BottomRTAS.hpp"
#include "TopRTAS.hpp"

#include <vector>
#include <memory>
#include <tuple>

namespace PathFinder
{

    struct GPUMeshInstanceTableEntry
    {
        glm::mat4 InstanceWorldMatrix;
        // 16 byte boundary
        glm::mat4 InstancePrevWorldMatrix;
        // 16 byte boundary
        glm::mat4 InstanceNormalMatrix;
        // 16 byte boundary
        uint32_t MaterialIndex;
        uint32_t UnifiedVertexBufferOffset;
        uint32_t UnifiedIndexBufferOffset;
        uint32_t IndexCount;
        // 16 byte boundary
        uint32_t HasTangentSpace;
    };

    struct GPUMaterialTableEntry
    {
        uint32_t AlbedoMapIndex;
        uint32_t NormalMapIndex;
        uint32_t RoughnessMapIndex;
        uint32_t MetalnessMapIndex;
        // 16 byte boundary
        uint32_t AOMapIndex;
        uint32_t DisplacementMapIndex;
        uint32_t DistanceFieldIndex;
        uint32_t LTC_LUT_MatrixInverse_Specular_Index;
        // 16 byte boundary
        uint32_t LTC_LUT_Matrix_Specular_Index;
        uint32_t LTC_LUT_Terms_Specular_Index;
        uint32_t LTC_LUT_MatrixInverse_Diffuse_Index;
        uint32_t LTC_LUT_Matrix_Diffuse_Index;
        // 16 byte boundary
        uint32_t LTC_LUT_Terms_Diffuse_Index;
        uint32_t LTC_LUT_TextureSize;
    };

    struct GPULightTableEntry
    {
        enum class LightType : uint32_t
        {
            Sphere = 0, Rectangle = 1, Disk = 2
        };

        glm::vec4 Orientation;
        // 16 byte boundary

        glm::vec4 Position;
        // 16 byte boundary

        glm::vec4 Color;
        // 16 byte boundary

        float Luminance;
        float Width;
        float Height;

        std::underlying_type_t<LightType> LightTypeRaw;
        // 16 byte boundary
    };

    struct GPULightTablePartitionInfo
    {
        uint32_t SphericalLightsOffset = 0;
        uint32_t EllipticalLightsOffset = 0;
        uint32_t RectangularLightsOffset = 0;
        uint32_t SphericalLightsCount = 0;
        uint32_t EllipticalLightsCount = 0;
        uint32_t RectangularLightsCount = 0;
        uint32_t TotalLightsCount = 0;
        uint32_t Pad1__;
    };

    struct GPUCamera
    {
        glm::vec4 Position;
        // 16 byte boundary
        glm::mat4 View;
        glm::mat4 Projection;
        glm::mat4 ViewProjection;
        glm::mat4 InverseView;
        glm::mat4 InverseProjection;
        glm::mat4 InverseViewProjection;
        // 16 byte boundary
        float NearPlane = 0.0f;
        float FarPlane = 0.0f;
        float ExposureValue100 = 0.0f;
        float FoVH = 0.0f;
        // 16 byte boundary
        float FoVV = 0.0f;
        float FoVHTan = 0.0f;
        float FoVVTan = 0.0f;
        float AspectRatio = 0;
        // 16 byte boundary
    };

    using GPUInstanceIndex = uint64_t;

    class SceneGPUStorage
    {
    public:
        SceneGPUStorage(Scene* scene, const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer);

        void UploadMeshes();
        void UploadMaterials();
        void UploadMeshInstances();
        void UploadLights();

        GPUCamera CameraGPURepresentation() const;

    private:
        template <class Vertex>
        struct UploadBufferPackage
        {
            std::vector<Vertex> Vertices;
            std::vector<uint32_t> Indices;
            std::vector<VertexStorageLocation> Locations;
        };

        template <class Vertex>
        struct FinalBufferPackage
        {
            Memory::GPUResourceProducer::BufferPtr VertexBuffer;
            Memory::GPUResourceProducer::BufferPtr IndexBuffer;
        };

        template <class Vertex>
        void SubmitTemporaryBuffersToGPU();

        GPULightTableEntry CreateLightGPUTableEntry(const FlatLight& light) const;
        GPULightTableEntry CreateLightGPUTableEntry(const SphericalLight& light) const;

        template <class Vertex>
        VertexStorageLocation WriteToTemporaryBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);

        std::tuple<UploadBufferPackage<Vertex1P1N1UV1T1BT>, UploadBufferPackage<Vertex1P1N1UV>, UploadBufferPackage<Vertex1P3>> mUploadBuffers;
        std::tuple<FinalBufferPackage<Vertex1P1N1UV1T1BT>, FinalBufferPackage<Vertex1P1N1UV>, FinalBufferPackage<Vertex1P3>> mFinalBuffers;

        std::vector<BottomRTAS> mBottomAccelerationStructures;
        TopRTAS mTopAccelerationStructure;

        Memory::GPUResourceProducer::BufferPtr mMeshInstanceTable;
        Memory::GPUResourceProducer::BufferPtr mLightTable;
        Memory::GPUResourceProducer::BufferPtr mMaterialTable;

        GPULightTablePartitionInfo mLightTablePartitionInfo;
        float mLightsMaximumLuminance;
        
        Scene* mScene;
        const HAL::Device* mDevice;
        Memory::GPUResourceProducer* mResourceProducer;

    public:
        inline const auto UnifiedVertexBuffer() const { return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).VertexBuffer.get(); }
        inline const auto UnifiedIndexBuffer() const { return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).IndexBuffer.get(); }
        inline const auto MeshInstanceTable() const { return mMeshInstanceTable.get(); }
        inline const auto LightTable() const { return mLightTable.get(); }
        inline const auto MaterialTable() const { return mMaterialTable.get(); }
        inline const auto& LightTablePartitionInfo() const { return mLightTablePartitionInfo; }
        inline const auto& TopAccelerationStructure() const { return mTopAccelerationStructure; }
        inline const auto& BottomAccelerationStructures() const { return mBottomAccelerationStructures; }
        inline auto LightsMaximumLuminanance() const { return mLightsMaximumLuminance; }
    };

}

#include "SceneGPUStorage.inl"
