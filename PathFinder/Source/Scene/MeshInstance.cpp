#include "MeshInstance.hpp"

namespace PathFinder
{

    MeshInstance::MeshInstance(const Mesh* mesh, const std::vector<GPUBufferLocation>& subMeshGPUBufferLocations)
        : mSubMeshGPUDataLocations(subMeshGPUBufferLocations)
    {

    }


    /*std::optional<MaterialReference> MeshInstance::materialReferenceForSubMeshID(ID subMeshID) const
    {
        if (mSubMeshMaterialMap.find(subMeshID) == mSubMeshMaterialMap.end()) {
            return std::nullopt;
        }
        return mSubMeshMaterialMap.at(subMeshID);
    }*/



    /*  void MeshInstance::setMaterialReferenceForSubMeshID(const MaterialReference &ref, ID subMeshID)
    {
        mSubMeshMaterialMap[subMeshID] = ref;
    }*/

}
