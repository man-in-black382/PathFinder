#pragma once

#include "ViewModel.hpp"
#include "UITextureData.hpp"

namespace PathFinder
{
   
    class TextureViewerViewModel : public ViewModel
    {
    public:
        struct PipelineTexture
        {
            std::string Name;
            Memory::Texture* Texture;
        };

        void Import() override;

    private:
        std::vector<PipelineTexture> mPipelineTextures;
        UITextureData mUITextureData;

    public:
        inline const auto& PipelineTextures() const { return mPipelineTextures; }
    };

}
