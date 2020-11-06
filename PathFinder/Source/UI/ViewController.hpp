#pragma once

#include "UIManager.hpp"
#include "ViewModel.hpp"

#include <IO/Input.hpp>
#include <RenderPipeline/PipelineResourceStorage.hpp>

namespace PathFinder
{

    class ViewController
    {
    public:
        virtual void Draw() = 0;
        virtual bool IsInteracting() const { return false; }
        virtual void OnCreated() {}

        void SetInput(const Input* input) { mInput = input; }
        void SetUIManager(UIManager* manager) { mUIManager = manager; }

    protected:
        template <class ViewModelT>
        ViewModelT* GetViewModel();

        template <class ViewControllerT>
        std::shared_ptr<ViewControllerT> CreateViewController();

        inline const Input* GetInput() const { return mInput; }
        inline const UIManager* GetUIManager() const { return mUIManager; }

    private:
        UIManager* mUIManager = nullptr;
        const Input* mInput = nullptr;
    };

    template <class ViewModelT>
    ViewModelT* ViewController::GetViewModel()
    {
        return mUIManager->GetViewModel<ViewModelT>();
    }

    template <class ViewControllerT>
    std::shared_ptr<ViewControllerT> ViewController::CreateViewController()
    {
        return mUIManager->CreateViewController<ViewControllerT>();
    }

}
