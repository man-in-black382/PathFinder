#include "ResourceNameResolving.hpp"

namespace PathFinder
{

    Foundation::Name SMAAEdgeDetectionPassInputSRName(bool isGIDebugEnabled)
    {
        return isGIDebugEnabled ? ResourceNames::GIDebugOutput : ResourceNames::BloomCompositionOutput;
    }

}
