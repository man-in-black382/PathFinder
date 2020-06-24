#include "PipelineResourceSchedulingInfo.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    PipelineResourceSchedulingInfo::PipelineResourceSchedulingInfo(Foundation::Name resourceName, const HAL::ResourceFormat& format)
        : mResourceFormat{ format }, mResourceName{ resourceName }, mSubresourceCount{ format.SubresourceCount() }
    {
        mSubresourceCombinedReadStates.resize(mSubresourceCount);
        mSubresourceWriteStates.resize(mSubresourceCount);
    }

    void PipelineResourceSchedulingInfo::AddExpectedStates(HAL::ResourceState states)
    {
        mExpectedStates |= states;
    }

    void PipelineResourceSchedulingInfo::FinishScheduling()
    {
        // Determine final memory requirements
        mResourceFormat.SetExpectedStates(mExpectedStates);
    }

    const PipelineResourceSchedulingInfo::PassInfo* PipelineResourceSchedulingInfo::GetInfoForPass(Foundation::Name passName) const
    {
        auto it = mPassInfoMap.find(passName);
        return it != mPassInfoMap.end() ? &it->second : nullptr;
    }

    PipelineResourceSchedulingInfo::PassInfo* PipelineResourceSchedulingInfo::GetInfoForPass(Foundation::Name passName)
    {
        auto it = mPassInfoMap.find(passName);
        return it != mPassInfoMap.end() ? &it->second : nullptr;
    }

    void PipelineResourceSchedulingInfo::SetSubresourceInfo(
        Foundation::Name passName,
        uint64_t subresourceIndex,
        HAL::ResourceState state,
        SubresourceInfo::AccessFlag accessFlag,
        std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        assert_format(subresourceIndex < mSubresourceCount, "Subresource index is out of bounds");

        PassInfo& passInfo = mPassInfoMap[passName];
        passInfo.SubresourceInfos.resize(mSubresourceCount);
        passInfo.SubresourceInfos[subresourceIndex] = SubresourceInfo{};
        passInfo.SubresourceInfos[subresourceIndex]->AccessValidationFlag = accessFlag;
        passInfo.SubresourceInfos[subresourceIndex]->RequestedState = state;
        passInfo.SubresourceInfos[subresourceIndex]->ShaderVisibleFormat = shaderVisibleFormat;

        if (IsResourceStateReadOnly(state))
        {
            mSubresourceCombinedReadStates[subresourceIndex] |= state;
        }
        else
        {
            assert_format(mSubresourceWriteStates[subresourceIndex] == HAL::ResourceState::Common,
                "One write state for subresource is already requested. Engine architecture allows one write per frame.");

            mSubresourceWriteStates[subresourceIndex] = state;

            if (EnumMaskBitSet(state, HAL::ResourceState::UnorderedAccess))
            {
                passInfo.NeedsUnorderedAccessBarrier = true;
            }
        }

        mExpectedStates |= state;
    }

    HAL::ResourceState PipelineResourceSchedulingInfo::GetSubresourceCombinedReadStates(uint64_t subresourceIndex) const
    {
        return mSubresourceCombinedReadStates[subresourceIndex];
    }

    HAL::ResourceState PipelineResourceSchedulingInfo::GetSubresourceWriteState(uint64_t subresourceIndex) const
    {
        return mSubresourceWriteStates[subresourceIndex];
    }

}
