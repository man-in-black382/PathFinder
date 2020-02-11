#include "PreprocessableAssetStorage.hpp"

namespace PathFinder
{

    void PreprocessableAssetStorage::PreprocessAsset(Memory::GPUResource* asset, const PostprocessCallback& callback)
    {
        mAssets.emplace_back(asset, callback);
    }

    void PreprocessableAssetStorage::ReadbackAllAssets()
    {
        for (auto& [asset, callback] : mAssets)
        {
            asset->RequestRead();
        }
    }

    void PreprocessableAssetStorage::ReportAllAssetsPostprocessed()
    {
        for (auto& [asset, callback] : mAssets)
        {
            callback(asset);
        }
        mAssets.clear();
    }

}
