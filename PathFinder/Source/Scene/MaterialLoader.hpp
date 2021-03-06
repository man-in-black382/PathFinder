#pragma once

#include "Material.hpp"
#include "ResourceLoader.hpp"

#include <HardwareAbstractionLayer/Buffer.hpp>
#include <Memory/GPUResourceProducer.hpp>
#include <robinhood/robin_hood.h>

#include <filesystem>
#include <string>
#include <optional>

namespace PathFinder 
{

    class MaterialLoader
    {
    public:
        MaterialLoader(const std::filesystem::path& executableFolderPath, Memory::GPUResourceProducer* resourceProducer);

        void LoadMaterial(Material& material);
        void SetCommonMaterialTextures(Material& material);

    private:
        Memory::Texture* GetOrAllocateTexture(const std::string& materialName, const std::filesystem::path& texturePath);
        Memory::Texture* AllocateAndStoreTexture(const HAL::TextureProperties& properties, const std::string& cacheKey);

        void CreateDefaultTextures();
        void LoadLTCLookupTables(const std::filesystem::path& executableFolderPath);

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
        ResourceLoader mResourceLoader;
    };

}
