#pragma once

#include <Memory/GPUResourceProducer.hpp>

#include <bitsery/bitsery.h>

namespace PathFinder 
{

    struct Material
    {
        Memory::Texture* AlbedoMap = nullptr;
        Memory::Texture* NormalMap = nullptr;
        Memory::Texture* RoughnessMap = nullptr;
        Memory::Texture* MetalnessMap = nullptr;
        Memory::Texture* AOMap = nullptr;
        Memory::Texture* DisplacementMap = nullptr;
        Memory::Texture* DistanceField = nullptr;
        Memory::Texture* LTC_LUT_MatrixInverse_Specular = nullptr;
        Memory::Texture* LTC_LUT_Matrix_Specular = nullptr;
        Memory::Texture* LTC_LUT_Terms_Specular = nullptr;
        Memory::Texture* LTC_LUT_MatrixInverse_Diffuse = nullptr;
        Memory::Texture* LTC_LUT_Matrix_Diffuse = nullptr;
        Memory::Texture* LTC_LUT_Terms_Diffuse = nullptr;

        std::string AlbedoMapPath;
        std::string NormalMapPath;
        std::string RoughnessMapPath;
        std::string MetalnessMapPath;
        std::string AOMapPath;
        std::string DisplacementMapPath;
        std::string DistanceFieldPath;

        uint32_t GPUMaterialTableIndex = 0;

        template <typename S>
        void serialize(S& s)
        {
            s.text(AlbedoMapPath);
            s.text(NormalMapPath);
            s.text(RoughnessMapPath);
            s.text(MetalnessMapPath);
            s.text(AOMapPath);
            s.text(DisplacementMapPath);
            s.text(DistanceFieldPath);
        }
    };

}
