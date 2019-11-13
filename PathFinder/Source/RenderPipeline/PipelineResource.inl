namespace PathFinder
{

    template <class ResourceT>
    typename const PipelineResource<ResourceT>::PassMetadata* PipelineResource<ResourceT>::GetMetadataForPass(Foundation::Name passName) const
    {
        auto it = mPerPassData.find(passName);
        return it == mPerPassData.end() ? nullptr : &it->second;
    }

    template <class ResourceT>
    typename PipelineResource<ResourceT>::PassMetadata& PipelineResource<ResourceT>::AllocateMetadateForPass(Foundation::Name passName)
    {
        auto [iter, success] = mPerPassData.emplace(passName, PassMetadata{});
        return iter->second;
    }

}
