#pragma once

#include "Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

// Assimp is in conflict with windows.h definitions of min and max
#ifndef NOMINMAX 
#define NOMINMAX
#endif

#ifdef min 
#undef min
#endif

#ifdef max 
#undef max
#endif

#include <vector>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace PathFinder 
{

    class ThirdPartySceneLoader
    {
    public:
        struct Settings
        {
            float InitialScale = 1.0;
        };

        struct LoadedMesh
        {
            Mesh MeshObject;
            uint64_t MaterialIndex = 0;
        };

        std::vector<LoadedMesh>& Load(const std::filesystem::path& path, const Settings& settings = {});

    private:
        void ProcessMaterials(const aiScene* scene);
        void ProcessMesh(Mesh& mesh, aiMesh* assimpMesh, const aiScene* scene);
        void ProcessNode(aiNode* node, const aiScene* scene);

        std::vector<Material> mLoadedMaterials;
        std::vector<LoadedMesh> mLoadedMeshes;
        std::filesystem::path mPath;
        std::filesystem::path mDirectory;
        Settings mLoadSettings;

    public:
        inline auto& LoadedMaterials() { return mLoadedMaterials; }
    };

}
