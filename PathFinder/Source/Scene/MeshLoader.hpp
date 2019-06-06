#pragma once

#include "Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "Mesh.hpp"

#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace PathFinder 
{

    class MeshLoader
    {
    public:
        MeshLoader(const std::filesystem::path& fileRoot);

        Mesh Load(const std::string& fileName);

    private:
        SubMesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
        void ProcessNode(aiNode* node, const aiScene* scene, Mesh& mesh);

        std::filesystem::path mRootPath;
    };

}
