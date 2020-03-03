namespace PathFinder
{

    template <class ConstantsType>
    void RootSignatureProxy::AddRootConstantsParameter(uint16_t bRegisterIndex, uint16_t registerSpace)
    {
        uint16_t num32BitValues = std::ceil(sizeof(ConstantsType) / 4.0f);
        mRootConstantsParameters.emplace_back(HAL::RootConstantsParameter{ num32BitValues, bRegisterIndex, registerSpace });
    }

}

