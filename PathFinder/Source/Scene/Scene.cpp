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

    Material& Scene::AddMaterial(Material&& material)
    {
        mMaterials.emplace_back(std::move(material));
        return mMaterials.back();
    }

    Scene::FlatLightIt Scene::EmplaceFlatLight(FlatLight::Type type)
    {
        mFlatLights.emplace_back(type);
        return std::prev(mFlatLights.end());
    }

    Scene::SphericalLightIt Scene::EmplaceSphericalLight()
    {
        mSphericalLights.emplace_back();
        return std::prev(mSphericalLights.end());
    }

    void Scene::IterateMeshInstances(const std::function<void(const MeshInstance & instance)>& functor) const
    {
        for (const MeshInstance& instance : mMeshInstances)
        {
            functor(instance);
        }
    }

    void Scene::IterateMeshInstances(const std::function<void(MeshInstance & instance)>& functor)
    {
        for (MeshInstance& instance : mMeshInstances)
        {
            functor(instance);
        }
    }

    void Scene::IterateMaterials(const std::function<void(const Material& material)>& functor) const
    {
        for (const Material& material : mMaterials)
        {
            functor(material);
        }
    }

}
