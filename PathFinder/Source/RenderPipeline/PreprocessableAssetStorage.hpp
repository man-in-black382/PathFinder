#pragma once

#include <Memory/GPUResource.hpp>

#include <functional>

namespace PathFinder
{

    class PreprocessableAssetStorage
    {
    public:
        using PostprocessCallback = std::function<void(Memory::GPUResource* asset)>;

        void PreprocessAsset(Memory::GPUResource* asset, const PostprocessCallback& callback);
        void ReadbackAllAssets();
        void ReportAllAssetsPostprocessed();

    private:
        std::vector<std::pair<Memory::GPUResource*, PostprocessCallback>> mAssets;
    };

}
