#pragma once

#include "../RenderPipeline/VertexStorage.hpp"

#include "Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "Mesh.hpp"

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

    class MeshLoader
    {
    public:
        MeshLoader(const std::filesystem::path& fileRoot, VertexStorage* gpuVertexStorage);

        std::vector<Mesh> Load(const std::string& fileName);

    private:
        Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
        void ProcessNode(aiNode* node, const aiScene* scene);

        std::vector<Mesh> mLoadedMeshes;
        std::filesystem::path mRootPath;
        VertexStorage* mVertexStorage;
    };

}
