#include "MeshInstance.hpp"

namespace PathFinder
{

    MeshInstance::MeshInstance(const Mesh* mesh, const Material* material)
        : mMesh{ mesh }, mMaterial{ material } {}

    GPUInstanceTableEntry MeshInstance::CreateGPUInstancetableEntry() const
    {
        return { mTransformation.ModelMatrix(), *mMaterial };
    }

}
