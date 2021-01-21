#include "ResourceNameResolving.hpp"

namespace PathFinder
{

    Foundation::Name ToneMappingPassInputSRName(bool isGIDebugEnabled)
    {
        return isGIDebugEnabled ? ResourceNames::GIDebugOutput : ResourceNames::BloomCompositionOutput;
    }

}
