#include "MeshInstance.hpp"

namespace PathFinder
{

    MeshInstance::MeshInstance(Mesh* mesh, Material* material)
        : mMesh{ mesh }, mMaterial{ material } {}

    void MeshInstance::UpdatePreviousFrameValues()
    {
        mPreviousTransformation = mTransformation;
    }

}
