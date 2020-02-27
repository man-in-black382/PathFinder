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
        case FlatLight::Type::Disk: lightType = GPULightInstanceTableEntry::LightType::Disk; break;
        case FlatLight::Type::Rectangle: lightType = GPULightInstanceTableEntry::LightType::Rectangle; break;
        }

        return{
                light.LuminousIntensity(),
                light.Width(),
                light.Height(),
                std::underlying_type_t<GPULightInstanceTableEntry::LightType>(lightType),
                glm::vec4(light.Normal(), 0.0f),
                glm::vec4(light.Position(), 1.0f),
                glm::vec4(light.Color().R(), light.Color().G(), light.Color().B(), 0.0f)
        };
    }

    PathFinder::GPULightInstanceTableEntry SceneGPUStorage::CreateLightGPUTableEntry(const SphericalLight& light) const
    {
        return{
                light.LuminousIntensity(),
                light.Radius(),
                light.Radius(),
                std::underlying_type_t<GPULightInstanceTableEntry::LightType>(GPULightInstanceTableEntry::LightType::Sphere),
                glm::vec4(0.0f), // No orientation required for spherical lights
                glm::vec4(light.Position(), 1.0f),
                glm::vec4(light.Color().R(), light.Color().G(), light.Color().B(), 0.0f)
        };
    }

}
