#pragma once

namespace PathFinder
{

    template <class ViewControllerT>
    std::shared_ptr<ViewControllerT> UIManager::CreateViewController()
    {
        static_assert(std::is_base_of<ViewController, ViewControllerT>::value, "ViewControllerT must derive from ViewController");

        auto vc = std::make_shared<ViewControllerT>();

        vc->SetInput(mInput);
        vc->SetUIManager(this);

        vc->OnCreated();

        mViewControllers.emplace_back(vc);

        return vc;
    }

    template <class ViewModelT>
    ViewModelT* UIManager::GetViewModel()
    {
        static_assert(std::is_base_of<ViewModel, ViewModelT>::value, "ViewModelT must derive from ViewModel");

        auto vmIt = mViewModels.find(typeid(ViewModelT));

        if (vmIt == mViewModels.end())
        {
            auto& vm = mViewModels.emplace(typeid(ViewModelT), std::make_unique<ViewModelT>()).first->second;
            vm->Dependencies = mUIDependencies;
            vm->OnCreated();
            return static_cast<ViewModelT*>(vm.get());
        }
        else
        {
            return static_cast<ViewModelT*>(vmIt->second.get());
        }
    }

}