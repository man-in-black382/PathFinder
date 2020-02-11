#pragma once

#include "Material.hpp"
#include "ResourceLoader.hpp"

#include "../RenderPipeline/PreprocessableAssetStorage.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"
#include "../Memory/GPUResourceProducer.hpp"

#include <filesystem>
#include <string>
#include <optional>

namespace PathFinder 
{

    class MaterialLoader
    {
    public:
        inline static const Geometry::Dimensions DistanceFieldTextureSize{ 128, 128, 64 };

        MaterialLoader(const std::filesystem::path& fileRoot, PreprocessableAssetStorage* assetStorage, Memory::GPUResourceProducer* resourceProducer);

        Material LoadMaterial(
            const std::string& albedoMapRelativePath,
            const std::string& normalMapRelativePath,
            const std::string& roughnessMapRelativePath,
            const std::string& metalnessMapRelativePath,
            std::optional<std::string> displacementMapRelativePath = std::nullopt,
            std::optional<std::string> distanceMapRelativePath = std::nullopt,
            std::optional<std::string> AOMapRelativePath = std::nullopt);

    private:
        struct SerializationData
        {
            std::string DistanceMapReltivePath;
            const HAL::Buffer* DistanceAtlasIndirectionBuffer;
            const HAL::Buffer* DistanceAtlasBuffer;
            const HAL::Buffer* DistanceAtlasCounterBuffer;
        };

        Memory::GPUResourceProducer* mResourceProducer;
        PreprocessableAssetStorage* mAssetStorage;
        ResourceLoader mResourceLoader;
    };

}
