#include "TextureViewerViewController.hpp"
#include "UIManager.hpp"

#include <fplus/fplus.hpp>

namespace PathFinder
{

    void TextureViewerViewController::OnCreated()
    {
        VM = GetViewModel<TextureViewerViewModel>();
    }

    void TextureViewerViewController::Draw()
    {
        VM->Import();

        ImGui::SetNextWindowSizeConstraints({ 300, 200 }, { 100000, 100000 });
        ImGui::Begin("Texture Viewer", nullptr, ImGuiWindowFlags_None);

        std::vector<const char*> namePtrs = 
            fplus::transform([](const TextureViewerViewModel::PipelineTexture& textureData) { return textureData.Name.c_str(); }, VM->PipelineTextures());

        if (!namePtrs.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::Combo("Pipeline Textures", &mSelectedPipelineTextureIndex, namePtrs.data(), namePtrs.size(), 20);
            ImGui::PopStyleColor();

            std::array<const char*, 2> samplingModeNames{ "Linear", "Point" };
            ImGui::Combo("Sampling Mode", (int32_t*)(&mSelectedTextureData.SamplerMode), samplingModeNames.data(), samplingModeNames.size());

            const Memory::Texture* texture = VM->PipelineTextures()[mSelectedPipelineTextureIndex].Texture;
            mSelectedTextureData.Texture = texture;
           
            float windowWidth = ImGui::GetWindowContentRegionWidth();
            float textureAspect = float(texture->Properties().Dimensions.Width) / texture->Properties().Dimensions.Height;
            float imageHeight = windowWidth / textureAspect;
            float imageWidth = windowWidth;

            ImGui::Image(&mSelectedTextureData, { imageWidth, imageHeight });
        }

        ImGui::End();

        VM->Export();
    }

}
