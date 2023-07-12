#include <EnginePCH.h>
#include <AssimpLoader.h>
#include <TypesCompat.h>
#include <Logger.h>

using namespace DirectX;

std::string GetDirectoryFromPath(const std::string& modelPath)
{
	return std::string(modelPath.begin(), modelPath.begin() + modelPath.find_last_of('/')) + "/";
}

std::string GetTextureName(aiTextureType type, aiMaterial* material)
{
	aiString aiTexName;
	material->GetTexture(type, 0, &aiTexName);
	std::filesystem::path p(aiTexName.C_Str());
	return p.filename().string();
}

void LogTexturePath(const std::string& texPath, aiTextureType type)
{
	LOG_INFO("Loading texture at path: %s", texPath.c_str());
	LOG_INFO("Texture Type: %s", aiTextureTypeToString(type));
	LOG_INFO("---------------------------------------------------------------");
}

void GetTexturePath(aiTextureType type, aiMaterial* material, const std::string& modelPath, std::string& outTexPath)
{
	if (material->GetTextureCount(type))
	{
		const auto directory = GetDirectoryFromPath(modelPath);
		const auto texName = GetTextureName(type, material);
		outTexPath = directory + texName;

		LogTexturePath(outTexPath, type);
	}
}

AssimpLoader::AssimpLoader(const std::string& path, MeshImport::Flags flags)
{
	LoadModel(path, flags);
}

void AssimpLoader::ProcessNode(aiNode* node, const aiScene* scene, MeshImport::Flags flags)
{
	for (auto i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		if (!LoadMesh(mesh, scene, flags))
			std::cerr << "failed to load mesh: " << path << std::endl;
	}
	for (auto i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, flags);
	}
}

template<class A, class B>
void AssimpToDirectX(A& a, const B& b)
{
	a.x = b.r;
	a.y = b.g;
	a.z = b.b;
}

bool AssimpLoader::LoadMesh(aiMesh* mesh, const aiScene* scene, MeshImport::Flags flags)
{
	AssimpMesh internalMesh;

	SetMeshName(internalMesh, mesh);
	LoadVertices(internalMesh, mesh, flags);
	LoadIndices(internalMesh, mesh);
	CheckForBones(mesh);
	LoadMaterials(internalMesh, mesh, scene, flags);
	
	staticMesh.meshes.emplace_back(internalMesh);

	return true;
}

void AssimpLoader::SetMeshName(AssimpMesh& internalMesh, aiMesh* mesh)
{
	internalMesh.name = mesh->mName.C_Str();
}

void AssimpLoader::LoadVertices(AssimpMesh& internalMesh, aiMesh* mesh, MeshImport::Flags flags)
{
	for (auto i = 0; i < mesh->mNumVertices; i++)
	{
		VertexPositionNormalTexture vertex;
		SetVertexPosition(vertex, mesh, i);
		if (mesh->HasNormals())
		{
			SetVertexNormal(vertex, mesh, i, flags);
		}
		if (mesh->mTextureCoords[0])
		{
			SetVertexTextureCoords(vertex, mesh, i);
		}
		if (mesh->HasTangentsAndBitangents())
		{
			SetVertexTangents(vertex, mesh, i);
		}
		internalMesh.vertices.emplace_back(vertex);
	}
}

void AssimpLoader::SetVertexPosition(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i)
{
	vertex.position.x = mesh->mVertices[i].x;
	vertex.position.y = mesh->mVertices[i].y;
	vertex.position.z = mesh->mVertices[i].z;
}

void AssimpLoader::SetVertexNormal(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i, MeshImport::Flags flags)
{
	bool bInvertNormals = flags & MeshImport::InvertNormals;
	vertex.normal.x = bInvertNormals ? -mesh->mNormals[i].x : mesh->mNormals[i].x;
	vertex.normal.y = bInvertNormals ? -mesh->mNormals[i].y : mesh->mNormals[i].y;
	vertex.normal.z = bInvertNormals ? -mesh->mNormals[i].z : mesh->mNormals[i].z;
}

void AssimpLoader::SetVertexTextureCoords(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i)
{
	vertex.textureCoordinate.x = mesh->mTextureCoords[0][i].x;
	vertex.textureCoordinate.y = mesh->mTextureCoords[0][i].y;
}

void AssimpLoader::SetVertexTangents(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i)
{
	vertex.tangent.x = mesh->mTangents[i].x;
	vertex.tangent.y = mesh->mTangents[i].y;
	vertex.tangent.z = mesh->mTangents[i].z;
	vertex.bitTangent.x = mesh->mBitangents[i].x;
	vertex.bitTangent.y = mesh->mBitangents[i].y;
	vertex.bitTangent.z = mesh->mBitangents[i].z;
}

void AssimpLoader::LoadIndices(AssimpMesh& internalMesh, aiMesh* mesh)
{
	for (auto i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (auto j = 0; j < face.mNumIndices; j++)
		{
			internalMesh.indices.push_back(face.mIndices[j]);
		}
	}
}

void AssimpLoader::CheckForBones(aiMesh* mesh)
{
	if (mesh->HasBones())
	{
		LOG_ERROR("Static mesh has bones, use skeletal mesh manager if you need bones!current path : %s", path.c_str());
	}
}

void AssimpLoader::LoadMaterials(AssimpMesh& internalMesh, aiMesh* mesh, const aiScene* scene, MeshImport::Flags flags)
{
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	if (!material) return;

	if (!(flags & MeshImport::SkipTextures))
	{
		LoadTextures(internalMesh, material, flags);
	}
	if (!(flags & MeshImport::IgnoreUserMaterial))
	{
		LoadUserMaterial(material, internalMesh.materialData, flags);
	}
}

void AssimpLoader::LoadTextures(AssimpMesh& internalMesh, aiMaterial* material, MeshImport::Flags flags)
{
	std::vector<aiTextureType> textureTypes = {
		aiTextureType_AMBIENT, aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR, aiTextureType_EMISSIVE,
		aiTextureType_HEIGHT, aiTextureType_NORMALS,
		aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS,
		aiTextureType_OPACITY
	};

	std::vector<AssimpMaterialType::Type> materialTypes = {
		AssimpMaterialType::Ambient, AssimpMaterialType::Diffuse, AssimpMaterialType::Specular,
		AssimpMaterialType::Emissive, AssimpMaterialType::Height, AssimpMaterialType::Normal,
		AssimpMaterialType::Metallic, AssimpMaterialType::Roughness, AssimpMaterialType::Opacity
	};

	for (size_t i = 0; i < textureTypes.size(); ++i)
	{
		GetTexturePath(textureTypes[i], material, path, internalMesh.material.textures[materialTypes[i]].path);
	}
}

void AssimpLoader::LoadModel(const std::string& path, MeshImport::Flags flags)
{
	if (!ValidatePath(path)) {
		return;
	}

	auto flagCopy = PrepareFlags(flags);

	Assimp::Importer importer;
	const aiScene* scene = LoadScene(importer, path, flagCopy);
	if (!ValidateScene(scene, importer)) {
		return;
	}

	this->path = path;
	ProcessNode(scene->mRootNode, scene, flags);
}

bool AssimpLoader::ValidatePath(const std::string& path)
{
	std::cout << "Loading model from path: " << path << std::endl;

	if (!std::filesystem::exists(path))
	{
		std::cout << "Failed to load model from path: " << path << std::endl;
		return false;
	}

	return true;
}

unsigned int AssimpLoader::PrepareFlags(MeshImport::Flags flags)
{
	auto flagCopy = aiFlags;

	if (flags & MeshImport::AssimpTangent)
		flagCopy |= aiProcess_CalcTangentSpace;

	return flagCopy;
}

const aiScene* AssimpLoader::LoadScene(Assimp::Importer& importer, const std::string& path, unsigned int flags)
{
	return importer.ReadFile(path, flags);
}

bool AssimpLoader::ValidateScene(const aiScene* scene, const Assimp::Importer& importer)
{
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "Assimp failed to load " << importer.GetErrorString() << std::endl;
		return false;
	}

	return true;
}

int ParseAsInt(const std::string& matkey, aiMaterial* material)
{
	ai_int  val;
	material->Get(matkey.c_str(), 0, 0, val);
	return val;
}

float ParseAsFloat(const std::string& matkey, aiMaterial* material)
{
	ai_real  val;
	material->Get(matkey.c_str(), 0, 0, val);
	return (float)val;
}

XMFLOAT3 PaseAsVec3(const std::string& matkey, aiMaterial* material)
{
	aiColor3D  val;
	material->Get(matkey.c_str(), 0, 0, val);
	return XMFLOAT3(val.r, val.b, val.g);
}

const AssimpStaticMesh& AssimpLoader::GetAssimpStaticMesh() const
{
	return staticMesh;
}

void AssimpLoader::LoadUserMaterial(aiMaterial* material, AssimpMaterialData& matData, MeshImport::Flags flags)
{
	LOG_INFO("Loading material %s", material->GetName().C_Str());
	//LOG( "-------------------------" );
	//LOG( "Loading material: " << material->GetName().C_Str() );
	//LOG( "Num properties: " << material->mNumProperties );

	for (size_t i = 0; i < material->mNumProperties; i++)
	{
		//LOG( material->mProperties[i]->mKey.C_Str() );

		std::string matKey = material->mProperties[i]->mKey.C_Str();

		if (matKey == "?mat.name")
		{
			aiString  matName;
			material->Get(AI_MATKEY_NAME, matName);

			std::cout << "MatName: ";
			std::cout << matName.C_Str() << std::endl;

			matData.name = matName.C_Str();
		}
		if (matKey == "$mat.bumpscaling")
		{
			matData.bumpScale = ParseAsFloat("$mat.bumpscaling", material);

			std::cout << "bumpscaling: " << matData.bumpScale << std::endl;
		}
		if (matKey == "$mat.blend")
		{
			matData.blendMode = ParseAsInt("$mat.blend", material);

			std::cout << "blendMode: " << matData.blendMode << std::endl;
		}
		if (matKey == "$clr.ambient")
		{
			matData.ambient = PaseAsVec3("$clr.ambient", material);

			std::cout << "ambient Color: " << matData.ambient.x << " " << matData.ambient.y << " " << matData.ambient.z << std::endl;
		}
		if (matKey == "$mat.transparencyfactor")
		{
			matData.alphaCutoff = ParseAsFloat("$mat.transparencyfactor", material);

			std::cout << "transparencyfactor: " << matData.alphaCutoff << std::endl;
		}
		if (matKey == "$mat.shadingm")
		{
			ai_int  shadinggm;
			material->Get(AI_MATKEY_SHADING_MODEL, shadinggm);

			std::cout << "shadingmodel: ";
			std::cout << shadinggm << std::endl;

			matData.shadingModel = shadinggm;
		}
		if (matKey == "$clr.diffuse" && !(flags & MeshImport::IgnoreUserMaterialAlbedoOnly))
		{
			aiColor3D color(0.f, 0.f, 0.f);
			material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

			AssimpToDirectX(matData.albedo, color);

			std::cout << "Diffuse Color: " << color.r << " " << color.g << " " << color.b << std::endl;

			matData.bHasMaterial = true;
		}
		if (matKey == "$clr.specular")
		{
			aiColor3D color(0.f, 0.f, 0.f);
			material->Get(AI_MATKEY_SPECULAR_FACTOR, color);

			AssimpToDirectX(matData.specular, color);

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
			matData.bHasRoughness = true;
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

			AssimpToDirectX(matData.emissive, color);

			std::cout << "Emissive Color: " << color.r << " " << color.g << " " << color.b << std::endl;

			matData.bHasMaterial = true;
		}
		if (matKey == "$clr.transparent")
		{
			aiColor3D color(0.f, 0.f, 0.f);
			material->Get(AI_MATKEY_COLOR_TRANSPARENT, color);

			AssimpToDirectX(matData.transparent, color);

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
		if (matKey == "$mat.twosided")
		{
			bool bTwoSided;
			material->Get(AI_MATKEY_TWOSIDED, bTwoSided);

			std::cout << "bTwoSided: " << bTwoSided << std::endl;

			matData.bTwoSided = bTwoSided;

			matData.bHasMaterial = true;
		}
		if (matKey == "$mat.gltf.alphaMode")
		{
			ai_int alphaMode;
			material->Get("$mat.gltf.alphaMode", 0, 0, alphaMode);

			std::cout << "alphaMode: " << alphaMode << std::endl;

			matData.alphaMode = alphaMode;

			matData.bHasMaterial = true;
		}
		if (matKey == "$mat.gltf.alphaCutoff")
		{
			float alphaCutoff;
			material->Get("$mat.gltf.alphaCutoff", 0, 0, alphaCutoff);

			std::cout << "alphaCutoff: " << alphaCutoff << std::endl;

			matData.alphaCutoff = alphaCutoff;

			matData.bHasMaterial = true;
		}
	}

	if (matData.bHasMaterial)
	{
		if (flags & MeshImport::ConvertShininiessToAlpha)
			if (matData.shininess > 0.f && !matData.bHasRoughness)
			{
				std::cout << "Converting Material Shininess: " << matData.name << " to roughness" << std::endl;

				std::cout << "Shininess value: " << matData.shininess << std::endl;

				matData.roughness = sqrt(ShininessToBeckmannAlpha(matData.shininess));

				std::cout << "New Roughness Value: " << matData.roughness << std::endl;
			}
	}

	std::cout << "-------------------------" << std::endl;
}
