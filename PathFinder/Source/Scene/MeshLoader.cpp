#include "MeshLoader.hpp"

namespace PathFinder
{
    
    MeshLoader::MeshLoader(const std::filesystem::path& fileRoot)
        : mRootPath(fileRoot) {}

    Mesh MeshLoader::Load(const std::string& fileName)
    {
        Assimp::Importer importer;
        std::string fullPath{ mRootPath.append(fileName).string() };

        auto postProcessSteps = (aiPostProcessSteps)(aiProcess_CalcTangentSpace | aiProcess_GenUVCoords | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
        const aiScene* pScene = importer.ReadFile(fullPath, postProcessSteps);

        if (!pScene) throw std::invalid_argument("Unable to read mesh file");

        Mesh mesh;
        ProcessNode(pScene->mRootNode, pScene, mesh);
        return mesh;
    }

    SubMesh MeshLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        SubMesh subMesh;

        // Walk through each of the mesh's vertices
        for (auto i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex1P1N1UV1T1BT vertex;

            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;

            if (mesh->HasTextureCoords(0))
            {
                vertex.UV.x = (float)mesh->mTextureCoords[0][i].x;
                vertex.UV.y = (float)mesh->mTextureCoords[0][i].y;
            }

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.Tangent.x = mesh->mTangents[i].x;
                vertex.Tangent.y = mesh->mTangents[i].y;
                vertex.Tangent.z = mesh->mTangents[i].z;

                vertex.Bitangent.x = mesh->mBitangents[i].x;
                vertex.Bitangent.y = mesh->mBitangents[i].y;
                vertex.Bitangent.z = mesh->mBitangents[i].z;
            }

            if (mesh->HasNormals())
            {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            }

            subMesh.AddVertex(vertex);
        }

        for (auto i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            for (auto j = 0; j < face.mNumIndices; j++) 
            {
                subMesh.AddIndex(face.mIndices[j]);
            }
        }

        subMesh.SetName(mesh->mName.data);

        return subMesh;
    }

    void MeshLoader::ProcessNode(aiNode* node, const aiScene* scene, Mesh& mesh)
    {
        for (auto i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
            mesh.AddSubMesh(ProcessMesh(assimpMesh, scene));
        }

        for (auto i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene, mesh);
        }
    }


}
