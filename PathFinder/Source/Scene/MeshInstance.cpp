#include "MeshInstance.hpp"

namespace PathFinder
{

    MeshInstance::MeshInstance(const Mesh* mesh, const Material* material)
        : mMesh{ mesh }, mMaterial{ material } {}

    void MeshInstance::UpdatePreviousTransform()
    {
        mPrevTransformation = mTransformation;
    }

}
