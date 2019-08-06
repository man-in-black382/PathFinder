#pragma once

#include "Mesh.hpp"
#include "MeshInstance.hpp"
#include "Camera.hpp"

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

        Camera mCamera;

    public:
        inline Camera& MainCamera() { return mCamera; }
        inline const Camera& MainCamera() const { return mCamera; }
    };

}
