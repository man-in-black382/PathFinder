#include "TextureViewerViewModel.hpp"

namespace PathFinder
{

    void TextureViewerViewModel::Import()
    {
        mPipelineTextures.clear();

        Dependencies->ResourceStorage->ForEachResource([this](const PipelineResourceStorageResource& pipelineResource)
        {
            if (Memory::Texture* texture = pipelineResource.Texture.get())
            {
                mPipelineTextures.emplace_back(PipelineTexture{ pipelineResource.ResourceName().ToString(), texture });
            }
        });

        std::sort(mPipelineTextures.begin(), mPipelineTextures.end(),
            [](auto& textureInfo1, auto& textureInfo2) { return textureInfo1.Name < textureInfo2.Name; });
    }

}
