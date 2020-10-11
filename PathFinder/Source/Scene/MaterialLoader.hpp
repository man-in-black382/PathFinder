#pragma once

#include "Material.hpp"
#include "ResourceLoader.hpp"

#include <RenderPipeline/PreprocessableAssetStorage.hpp>
#include <HardwareAbstractionLayer/Buffer.hpp>
#include <Memory/GPUResourceProducer.hpp>

#include <filesystem>
#include <string>
#include <optional>

namespace PathFinder 
{

    class MaterialLoader
    {
    public:
        inline static const Geometry::Dimensions DistanceFieldTextureSize{ 128, 128, 64 };

        MaterialLoader(const std::filesystem::path& executableFolder, PreprocessableAssetStorage* assetStorage, Memory::GPUResourceProducer* resourceProducer);

        Material LoadMaterial(
            const std::string& albedoMapRelativePath,
            const std::string& normalMapRelativePath,
            std::optional<std::string> roughnessMapRelativePath = std::nullopt,
            std::optional<std::string> metalnessMapRelativePath = std::nullopt,
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

        Memory::Texture* GetOrAllocateTexture(const std::string& relativePath);
        Memory::Texture* AllocateAndStoreTexture(const HAL::TextureProperties& properties, const std::string& relativePath);

        void CreateDefaultTextures();
        void LoadLTCLookupTables();

        std::unordered_map<std::string, Memory::GPUResourceProducer::TexturePtr> mMaterialTextures;
        Memory::GPUResourceProducer::TexturePtr m1x1Black2DTexture;
        Memory::GPUResourceProducer::TexturePtr m1x1White2DTexture;
        Memory::GPUResourceProducer::TexturePtr m1x1Black3DTexture;

        Memory::GPUResourceProducer::TexturePtr mLTC_LUT_MatrixInverse_GGXHeightCorrelated;
        Memory::GPUResourceProducer::TexturePtr mLTC_LUT_Matrix_GGXHeightCorrelated;
        Memory::GPUResourceProducer::TexturePtr mLTC_LUT_Terms_GGXHeightCorrelated;

        Memory::GPUResourceProducer::TexturePtr mLTC_LUT_MatrixInverse_DisneyDiffuseNormalized;
        Memory::GPUResourceProducer::TexturePtr mLTC_LUT_Matrix_DisneyDiffuseNormalized;
        Memory::GPUResourceProducer::TexturePtr mLTC_LUT_Terms_DisneyDiffuseNormalized;

        Memory::GPUResourceProducer* mResourceProducer;
        PreprocessableAssetStorage* mAssetStorage;
        ResourceLoader mResourceLoader;
    };

}
