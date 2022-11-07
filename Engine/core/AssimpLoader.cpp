#include <EnginePCH.h>
#include <AssimpLoader.h>

using namespace DirectX;

void GetTexturePath(aiTextureType type, aiMaterial* material, const std::string& modelPath, std::string& outTexPath)
{
    if (material->GetTextureCount(type))
    {
        aiString aiTexName;
        material->GetTexture(type, 0, &aiTexName);
        std::filesystem::path p(aiTexName.C_Str());
        auto texName = p.filename().string();
        outTexPath = std::string(modelPath.begin(), modelPath.begin() + modelPath.find_last_of('/')) + "/" + texName;
        std::cout << "Tex path: " << outTexPath << std::endl;
    }
}

AssimpLoader::AssimpLoader(const std::string& path)
{
    LoadModel(path);
}

void AssimpLoader::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (auto i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (!LoadMesh(mesh, scene))
            std::cerr << "failed to load mesh: " << path << std::endl;
    }
    for (auto i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

bool AssimpLoader::LoadMesh(aiMesh* mesh, const aiScene* scene)
{
    AssimpLoader::AssimpMesh internalMesh;

    for (auto i = 0; i < mesh->mNumVertices; i++)
    {
        AssimpVertex vertex;

        {	//Position and normals

            vertex.pos.x = mesh->mVertices[i].x;
            vertex.pos.y = mesh->mVertices[i].y;
            vertex.pos.z = mesh->mVertices[i].z;
            
            if (mesh->HasNormals())
            { 
                vertex.norm.x = mesh->mNormals[i].x;
                vertex.norm.y = mesh->mNormals[i].y;
                vertex.norm.z = mesh->mNormals[i].z;
            }
        }
        {	//Texture coords
            if (mesh->mTextureCoords[0])
            {
                vertex.uv.x = mesh->mTextureCoords[0][i].x;
                vertex.uv.y = mesh->mTextureCoords[0][i].y;                
            }
        }
        {	//Tangents
            if (mesh->HasTangentsAndBitangents())
            { 
                vertex.tangent.x = mesh->mTangents[i].x;
                vertex.tangent.y = mesh->mTangents[i].y;
                vertex.tangent.z = mesh->mTangents[i].z;

                vertex.bitTangent.x = mesh->mBitangents[i].x;
                vertex.bitTangent.y = mesh->mBitangents[i].y;
                vertex.bitTangent.z = mesh->mBitangents[i].z;

            }
                
        }
        internalMesh.vertices.emplace_back(vertex);
    }
    {	//Indices
        for (auto i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (auto j = 0; j < face.mNumIndices; j++)
                internalMesh.indices.push_back(face.mIndices[j]);
        }
    }
    {	//Bonedata
        if (mesh->HasBones())
        {
            std::cerr << "Static mesh has bones, use skeletal mesh manager if you need bones! current path: " << path << std::endl;
        }
    }
    { // Materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        GetTexturePath(aiTextureType::aiTextureType_AMBIENT, material, path, internalMesh.material.textures[AssimpMaterialType::Ambient].path);

        GetTexturePath(aiTextureType::aiTextureType_DIFFUSE, material, path, internalMesh.material.textures[AssimpMaterialType::Diffuse].path);

        GetTexturePath(aiTextureType::aiTextureType_SPECULAR, material, path, internalMesh.material.textures[AssimpMaterialType::Specular].path);

        GetTexturePath(aiTextureType::aiTextureType_EMISSIVE, material, path, internalMesh.material.textures[AssimpMaterialType::Emissive].path);

        GetTexturePath(aiTextureType::aiTextureType_NORMALS, material, path, internalMesh.material.textures[AssimpMaterialType::Normal].path);

        GetTexturePath(aiTextureType::aiTextureType_METALNESS, material, path, internalMesh.material.textures[AssimpMaterialType::Metallic].path);

        GetTexturePath(aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS, material, path, internalMesh.material.textures[AssimpMaterialType::Roughness].path);

        GetTexturePath(aiTextureType::aiTextureType_OPACITY, material, path, internalMesh.material.textures[AssimpMaterialType::Opacity].path);
        
    }

    staticMesh.meshes.emplace_back(internalMesh);
    return true;
}

void AssimpLoader::LoadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene;

    std::cout << "Loading model from path: " << path << std::endl;

    if (!std::filesystem::exists(path))
    {
        std::cout << "Failed to load model from path: " << path << std::endl;
        // throw std::exception("Failed to find Assimp Model Path!");
    }

    scene = importer.ReadFile(path, aiFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        std::cout << "Assimp failed to load " << importer.GetErrorString() << std::endl;
        return;
    }

    this->path = path;

    ProcessNode(scene->mRootNode, scene);
}
