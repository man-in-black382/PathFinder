#pragma once

#include "ViewController.hpp"
#include "TextureViewerViewModel.hpp"

namespace PathFinder
{
   
    class TextureViewerViewController : public ViewController
    {
    public:
        void Draw() override;
        void OnCreated() override;

    private:
        TextureViewerViewModel* VM;
        int32_t mSelectedPipelineTextureIndex = 0;
        UITextureData mSelectedTextureData;
    };

}
