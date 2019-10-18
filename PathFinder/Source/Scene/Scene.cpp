#include "Scene.hpp"

namespace PathFinder 
{

    Mesh& Scene::AddMesh(Mesh&& mesh)
    {
        mMeshes.emplace_back(std::move(mesh));
        return mMeshes.back();
    }

    MeshInstance& Scene::AddMeshInstance(MeshInstance&& instance)
    {
        mMeshInstances.emplace_back(std::move(instance));
        return mMeshInstances.back();
    }

    PathFinder::Material& Scene::AddMaterial(Material&& instance)
    {

    }

    void Scene::IterateMeshInstances(const std::function<void(const MeshInstance& instance)>& functor) const
    {
        for (const MeshInstance& instance : mMeshInstances)
        {
            functor(instance);
        }
    }

    void Scene::IterateSubMeshes(const Mesh& mesh, const std::function<void(const SubMesh& subMesh)>& functor) const
    {
        for (const SubMesh& subMesh : mesh.SubMeshes())
        {
            functor(subMesh);
        }
    }

}
