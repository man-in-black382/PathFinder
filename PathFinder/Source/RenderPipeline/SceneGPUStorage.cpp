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

    GPULightTableEntry SceneGPUStorage::CreateLightGPUTableEntry(const FlatLight& light) const
    {
        GPULightTableEntry::LightType lightType{};

        switch (light.LightType())
        {
        case FlatLight::Type::Disk: lightType = GPULightTableEntry::LightType::Disk; break;
        case FlatLight::Type::Rectangle: lightType = GPULightTableEntry::LightType::Rectangle; break;
        }

        return{
                glm::vec4(light.Normal(), 0.0f),
                glm::vec4(light.Position(), 1.0f),
                glm::vec4(light.Color().R(), light.Color().G(), light.Color().B(), 0.0f),
                light.LuminousIntensity(),
                light.Luminance(),
                light.Width(),
                light.Height(),
                std::underlying_type_t<GPULightTableEntry::LightType>(lightType)
        };
    }

    GPULightTableEntry SceneGPUStorage::CreateLightGPUTableEntry(const SphericalLight& light) const
    {
        return{
                glm::vec4(0.0f), // No orientation required for spherical lights
                glm::vec4(light.Position(), 1.0f),
                glm::vec4(light.Color().R(), light.Color().G(), light.Color().B(), 0.0f),
                light.LuminousIntensity(),
                light.Luminance(),
                light.Radius(),
                light.Radius(),
                std::underlying_type_t<GPULightTableEntry::LightType>(GPULightTableEntry::LightType::Sphere)
        };
    }

}
