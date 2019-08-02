#pragma once

#include "Mesh.hpp"
#include "MeshInstance.hpp"

#include <functional>
#include <list>
#include <memory>

namespace PathFinder 
{

    class Scene 
    {
    public:
        const Mesh& AddMesh(const Mesh& mesh);
        const Mesh& AddMesh(Mesh&& mesh);
        const MeshInstance& AddMeshInstance(const MeshInstance& instance);
        const MeshInstance& AddMeshInstance(MeshInstance&& instance);

        void IterateMeshInstances(const std::function<void(const MeshInstance& instance)>& functor) const;
        void IterateSubMeshes(const Mesh& mesh, const std::function<void(const SubMesh& subMesh)>& functor) const;

    private:
        std::list<Mesh> mMeshes;
        std::list<MeshInstance> mMeshInstances;

    public:
        
    };

}
