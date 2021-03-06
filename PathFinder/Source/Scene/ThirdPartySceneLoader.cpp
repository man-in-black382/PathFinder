#include "ThirdPartySceneLoader.hpp"

namespace PathFinder
{

    std::vector<ThirdPartySceneLoader::LoadedMesh>& ThirdPartySceneLoader::Load(const std::filesystem::path& path)
    {
        mPath = path;
        mDirectory = path.parent_path();

        Assimp::Importer importer;

        auto postProcessSteps = (aiPostProcessSteps)(
            aiProcess_Triangulate |
            aiProcess_CalcTangentSpace | 
            aiProcess_FlipUVs |
            aiProcess_GenUVCoords | 
            aiProcess_GenNormals | 
            aiProcess_GenSmoothNormals | 
            aiProcess_JoinIdenticalVertices |
            aiProcess_ConvertToLeftHanded);

        const aiScene* pScene = importer.ReadFile(path.string(), postProcessSteps);

        assert_format(pScene, "Unable to read mesh file (", path.string(), ")");

        mLoadedMeshes.clear();
        mLoadedMaterials.clear();

        ProcessMaterials(pScene);
        ProcessNode(pScene->mRootNode, pScene);

        return mLoadedMeshes;
    }

    void ThirdPartySceneLoader::ProcessMaterials(const aiScene* scene)
    {
        if (!scene->HasMaterials())
            return;

        for (auto matIdx = 0u; matIdx < scene->mNumMaterials; ++matIdx)
        {
            Material& material = mLoadedMaterials.emplace_back();
            aiMaterial* assimpMaterial = scene->mMaterials[matIdx];

            aiTextureMapMode wrapMode = aiTextureMapMode_Wrap;
            aiString texturePath;
            aiColor4D color;

            auto aiWrapModeToWrapMode = [&wrapMode](Material::TextureData& textureData)
            {
                switch (wrapMode)
                {
                case aiTextureMapMode_Wrap: textureData.Wrapping = Material::WrapMode::Repeat; break;
                case aiTextureMapMode_Mirror: textureData.Wrapping = Material::WrapMode::Mirror; break;
                default:textureData.Wrapping = Material::WrapMode::Clamp; break;
                }
            };

            material.Name = assimpMaterial->GetName().C_Str();

            if (assimpMaterial->GetTextureCount(aiTextureType_DIFFUSE))
            {
                assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, &wrapMode);
                material.DiffuseAlbedoMap.FilePath = mDirectory / texturePath.C_Str();
            }
            else if (aiGetMaterialColor(assimpMaterial, AI_MATKEY_COLOR_DIFFUSE, &color) == aiReturn_SUCCESS)
            {
                material.DiffuseAlbedoOverride = { color.r, color.g, color.b };
            }

            aiWrapModeToWrapMode(material.DiffuseAlbedoMap);

            if (assimpMaterial->GetTextureCount(aiTextureType_SPECULAR))
            {
                assimpMaterial->GetTexture(aiTextureType_SPECULAR, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, &wrapMode);
                material.SpecularAlbedoMap.FilePath = mDirectory / texturePath.C_Str();
            }
            else if (aiGetMaterialColor(assimpMaterial, AI_MATKEY_COLOR_SPECULAR, &color) == aiReturn_SUCCESS)
            {
                material.SpecularAlbedoOverride = { color.r, color.g, color.b };
            }
            else
            {
                material.SpecularAlbedoMap.FilePath = material.DiffuseAlbedoMap.FilePath;
            }

            aiWrapModeToWrapMode(material.SpecularAlbedoMap);
            
            if (assimpMaterial->GetTextureCount(aiTextureType_OPACITY))
            {
                assimpMaterial->GetTexture(aiTextureType_OPACITY, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, &wrapMode);
                material.TranslucencyMap.FilePath = mDirectory / texturePath.C_Str();
            }
            else if (aiGetMaterialColor(assimpMaterial, AI_MATKEY_OPACITY, &color) == aiReturn_SUCCESS)
            {
                material.TranslucencyOverride = 1.0 - color.r;
            }

            aiWrapModeToWrapMode(material.TranslucencyMap);

            if (assimpMaterial->GetTextureCount(aiTextureType_NORMALS))
            {
                assimpMaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, &wrapMode);
                material.NormalMap.FilePath = mDirectory / texturePath.C_Str();
            }

            aiWrapModeToWrapMode(material.NormalMap);

            if (assimpMaterial->GetTextureCount(aiTextureType_HEIGHT))
            {
                assimpMaterial->GetTexture(aiTextureType_HEIGHT, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, &wrapMode);
                material.DisplacementMap.FilePath = mDirectory / texturePath.C_Str();
            }

            aiWrapModeToWrapMode(material.DisplacementMap);

            if (aiGetMaterialColor(assimpMaterial, AI_MATKEY_REFRACTI, &color) == aiReturn_SUCCESS)
                material.IOROverride = color.r;
        }
    }

    void ThirdPartySceneLoader::ProcessMesh(Mesh& mesh, aiMesh* assimpMesh, const aiScene* scene)
    {
        // Walk through each of the mesh's vertices
        for (auto i = 0u; i < assimpMesh->mNumVertices; i++)
        {
            Vertex1P1N1UV1T1BT vertex{};

            vertex.Position.x = assimpMesh->mVertices[i].x;
            vertex.Position.y = assimpMesh->mVertices[i].y;
            vertex.Position.z = assimpMesh->mVertices[i].z;
            vertex.Position.w = 1.0; 

            if (assimpMesh->HasTextureCoords(0))
            {
                vertex.UV.x = (float)assimpMesh->mTextureCoords[0][i].x;
                vertex.UV.y = (float)assimpMesh->mTextureCoords[0][i].y;
            }

            if (assimpMesh->HasTangentsAndBitangents())
            {
                vertex.Tangent.x = assimpMesh->mTangents[i].x;
                vertex.Tangent.y = assimpMesh->mTangents[i].y;
                vertex.Tangent.z = assimpMesh->mTangents[i].z;

                vertex.Bitangent.x = assimpMesh->mBitangents[i].x;
                vertex.Bitangent.y = assimpMesh->mBitangents[i].y;
                vertex.Bitangent.z = assimpMesh->mBitangents[i].z;
            }

            if (assimpMesh->HasNormals())
            {
                vertex.Normal.x = assimpMesh->mNormals[i].x;
                vertex.Normal.y = assimpMesh->mNormals[i].y;
                vertex.Normal.z = assimpMesh->mNormals[i].z;
            }

            mesh.AddVertex(vertex);
        }

        for (auto i = 0u; i < assimpMesh->mNumFaces; i++)
        {
            aiFace face = assimpMesh->mFaces[i];

            for (auto j = 0u; j < face.mNumIndices; j++) 
            {
                mesh.AddIndex(face.mIndices[j]);
            }
        }

        mesh.SetName(assimpMesh->mName.data);
    }

    void ThirdPartySceneLoader::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for (auto i = 0u; i < node->mNumMeshes; i++)
        {
            aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
            LoadedMesh& loadedMesh = mLoadedMeshes.emplace_back();
            ProcessMesh(loadedMesh.MeshObject, assimpMesh, scene);
            loadedMesh.MaterialIndex = assimpMesh->mMaterialIndex;
        }

        for (auto i = 0u; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }

}
