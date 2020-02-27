#include "SceneGPUStorage.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    SceneGPUStorage::SceneGPUStorage(const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer)
        : mDevice{ device }, mResourceProducer{ resourceProducer }, mTopAccelerationStructure{ device, resourceProducer } 
    {
        mTopAccelerationStructure.SetDebugName("All Meshes Top RT AS");
    }        

    void SceneGPUStorage::ClearMeshInstanceTable()
    {
        mUploadedMeshInstances = 0;
    }

    void SceneGPUStorage::ClearLightInstanceTable()
    {
        mUploadedLights = 0;
    }

    GPULightInstanceTableEntry SceneGPUStorage::CreateLightGPUTableEntry(const FlatLight& light) const
    {
        GPULightInstanceTableEntry::LightType lightType{};

        switch (light.LightType())
        {
        case FlatLight::Type::Disk:
        }

        return{
                light.LuminousIntensity(),
                light.Width(),
                light.Height(),
                0.0f, // 0 radius for disk lights: unused
                glm::vec4(light.Normal(), 0.0f),
                glm::vec4(light.Position(), 0.0f),
                glm::vec4(light.Color().R(), light.Color().G(), light.Color().B(), 0.0f),
                std::underlying_type_t<GPULightInstanceTableEntry::LightType>(GPULightInstanceTableEntry::LightType::Disk)
        };
    }

}
