namespace PathFinder
{

    template <class Constants>
    void RenderEngine::SetFrameRootConstants(const Constants& constants)
    {
        mPipelineResourceStorage.UpdateFrameRootConstants(constants);
    }

    template <class Constants>
    void RenderEngine::SetGlobalRootConstants(const Constants& constants)
    {
        mPipelineResourceStorage.UpdateGlobalRootConstants(constants);
    }

}

