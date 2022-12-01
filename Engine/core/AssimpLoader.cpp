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
   
        std::cout << "Loading texture at path: " << outTexPath << std::endl;
        std::cout << "Texture Type: " << aiTextureTypeToString(type) << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;
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

template<class A, class B>
void AssToDX(A& a, B& b)
{
    a.x = b.r;
    a.y = b.g;
    a.z = b.b;
}

bool AssimpLoader::LoadMesh(aiMesh* mesh, const aiScene* scene)
{
    AssimpLoader::AssimpMesh internalMesh;

    internalMesh.name = mesh->mName.C_Str();

    for (auto i = 0; i < mesh->mNumVertices; i++)
    {
        VertexPositionNormalTexture vertex;

        {	//Position and normals
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;
            
            if (mesh->HasNormals())
            { 
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }
        }
        {	//Texture coords
            if (mesh->mTextureCoords[0])
            {
                vertex.textureCoordinate.x = mesh->mTextureCoords[0][i].x;
                vertex.textureCoordinate.y = mesh->mTextureCoords[0][i].y;
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

        AssimpMaterialData matData;

        std::cout << "Loading material: " << material->GetName().C_Str() << std::endl;
        std::cout << "Num properties: " << material->mNumProperties << std::endl;

        for (size_t i = 0; i < material->mNumProperties; i++)
        {
            std::cout << material->mProperties[i]->mKey.C_Str() << std::endl;

            std::string matKey = material->mProperties[i]->mKey.C_Str();

            if (matKey == "?mat.name")
            {
                aiString  matName;
                material->Get(AI_MATKEY_NAME, matName);

                std::cout << "MatName: ";
                std::cout << matName.C_Str() << std::endl;

                matData.name = matName.C_Str();
            }
            if (matKey == "$mat.shadingm")
            {
                ai_int  shadinggm;
                material->Get(AI_MATKEY_SHADING_MODEL, shadinggm);

                std::cout << "shadingmodel: ";
                std::cout << shadinggm << std::endl;

                matData.shadingModel = shadinggm;
            }
            if (matKey == "$clr.diffuse")
            { 
                aiColor3D color(0.f, 0.f, 0.f);
                material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

                AssToDX(matData.albedo, color);

                std::cout << "Diffuse Color: " << color.r << " " << color.g << " " << color.b << std::endl;

                matData.bHasMaterial = true;
            }
            if (matKey == "$clr.specular")
            {
                aiColor3D color(0.f, 0.f, 0.f);
                material->Get(AI_MATKEY_SPECULAR_FACTOR, color);

                AssToDX(matData.specular, color);

                std::cout << "Specular Color: " << color.r << " " << color.g << " " << color.b << std::endl;

                matData.bHasMaterial = true;
            }
            if (matKey == "$mat.roughnessFactor")
            {
                ai_real roughness;
                material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

                matData.roughness = roughness;

                std::cout << "roughness: " << roughness << std::endl;

                matData.bHasMaterial = true;
            }
            if (matKey == "$mat.metallicFactor")
            {
                ai_real metallic;
                material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);

                matData.metallic = metallic;

                std::cout << "metallic: " << metallic << std::endl;

                matData.bHasMaterial = true;
            }
            if (matKey == "$mat.shininess")
            {
                ai_real shininess;
                material->Get(AI_MATKEY_SHININESS, shininess);

                matData.shininess = shininess;

                std::cout << "shininess: " << shininess << std::endl;

                matData.bHasMaterial = true;
            }
            if (matKey == "$clr.emissive")
            {
                aiColor3D color(0.f, 0.f, 0.f);
                material->Get(AI_MATKEY_COLOR_EMISSIVE, color);

                AssToDX(matData.emissive, color);

                std::cout << "Emissive Color: " << color.r << " " << color.g << " " << color.b << std::endl;

                matData.bHasMaterial = true;
            }
            if (matKey == "$clr.transparent")
            {
                aiColor3D color(0.f, 0.f, 0.f);
                material->Get(AI_MATKEY_COLOR_TRANSPARENT, color);

                AssToDX(matData.transparent, color);

                std::cout << "Transparent Color: " << color.r << " " << color.g << " " << color.b << std::endl;

                matData.bHasMaterial = true;
            }
            if (matKey == "$mat.opacity")
            {
                ai_real opacity;
                material->Get(AI_MATKEY_OPACITY, opacity);

                matData.opacity = opacity;

                std::cout << "Opacity: " << opacity << std::endl;

                matData.bHasMaterial = true;
            }
        }

        if (matData.bHasMaterial)
        {        
            internalMesh.materialData = matData;
        }

        GetTexturePath(aiTextureType::aiTextureType_AMBIENT, material, path, internalMesh.material.textures[AssimpMaterialType::Ambient].path);

        GetTexturePath(aiTextureType::aiTextureType_DIFFUSE, material, path, internalMesh.material.textures[AssimpMaterialType::Diffuse].path);

        GetTexturePath(aiTextureType::aiTextureType_SPECULAR, material, path, internalMesh.material.textures[AssimpMaterialType::Specular].path);

        GetTexturePath(aiTextureType::aiTextureType_EMISSIVE, material, path, internalMesh.material.textures[AssimpMaterialType::Emissive].path);

        GetTexturePath(aiTextureType::aiTextureType_HEIGHT, material, path, internalMesh.material.textures[AssimpMaterialType::Height].path);

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
