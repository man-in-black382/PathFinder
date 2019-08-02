#include "Scene.hpp"

namespace PathFinder 
{

    const Mesh& Scene::AddMesh(const Mesh& mesh)
    {
        mMeshes.push_back(mesh);
        return mMeshes.back();
    }

    const Mesh& Scene::AddMesh(Mesh&& mesh)
    {
        mMeshes.emplace_back(std::move(mesh));
        return mMeshes.back();
    }

    const MeshInstance& Scene::AddMeshInstance(const MeshInstance& instance)
    {
        mMeshInstances.push_back(instance);
        return mMeshInstances.back();
    }

    const MeshInstance& Scene::AddMeshInstance(MeshInstance&& instance)
    {
        mMeshInstances.emplace_back(std::move(instance));
        return mMeshInstances.back();
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
