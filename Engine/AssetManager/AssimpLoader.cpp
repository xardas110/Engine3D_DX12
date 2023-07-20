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

AssimpLoader::AssimpLoader(const std::string& path, MeshFlags::Flags flags)
{
	LoadModel(path, flags);
}

void AssimpLoader::ProcessNode(aiNode* node, const aiScene* scene, MeshFlags::Flags flags)
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

bool AssimpLoader::LoadMesh(aiMesh* mesh, const aiScene* scene, MeshFlags::Flags flags)
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

void AssimpLoader::LoadVertices(AssimpMesh& internalMesh, aiMesh* mesh, MeshFlags::Flags flags)
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

void AssimpLoader::SetVertexNormal(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i, MeshFlags::Flags flags)
{
	bool bInvertNormals = flags & MeshFlags::InvertNormals;
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
		LOG_ERROR("Static mesh has bones, use skeletal mesh manager if you need bones! Current path : %s", path.c_str());
	}
}

void AssimpLoader::LoadMaterials(AssimpMesh& internalMesh, aiMesh* mesh, const aiScene* scene, MeshFlags::Flags flags)
{
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	if (!material) return;

	if (!(flags & MeshFlags::SkipTextures))
	{
		LoadTextures(internalMesh, material, flags);
	}
	if (!(flags & MeshFlags::IgnoreUserMaterial))
	{
		LoadUserMaterial(material, internalMesh.materialData, flags);
	}
}

void AssimpLoader::LoadTextures(AssimpMesh& internalMesh, aiMaterial* material, MeshFlags::Flags flags)
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

void AssimpLoader::LoadModel(const std::string& path, MeshFlags::Flags flags)
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

unsigned int AssimpLoader::PrepareFlags(MeshFlags::Flags flags)
{
	auto flagCopy = aiFlags;

	if (flags & MeshFlags::AssimpTangent)
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

XMFLOAT3 ParseAsVec3(const std::string& matkey, aiMaterial* material)
{
	aiColor3D  val;
	material->Get(matkey.c_str(), 0, 0, val);
	return XMFLOAT3(val.r, val.b, val.g);
}

const AssimpStaticMesh& AssimpLoader::GetAssimpStaticMesh() const
{
	return staticMesh;
}

void AssimpLoader::LoadUserMaterial(aiMaterial* material, AssimpMaterialData& matData, MeshFlags::Flags flags)
{
	LOG_INFO("Loading material %s", material->GetName().C_Str());

	using HandlerFunction = std::function<void(aiMaterial*, const std::string&, AssimpMaterialData&)>;
	static std::unordered_map<std::string, HandlerFunction> materialKeyToFunction = {

		{"?mat.name", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleMaterialName(mat, key, data); }},

		{"$mat.bumpscaling", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleBumpscaling(mat, key, data); }},

		{"$mat.blend", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleBlendMode(mat, key, data); }},

		{"$mat.ambient", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleAmbient(mat, key, data); }},

		{"$mat.transparencyfactor", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleTransparencyFactor(mat, key, data); }},

		{"$mat.shadingm", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleShadingModel(mat, key, data); }},

		{"$clr.diffuse", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleDiffuseColor(mat, key, data, flags); }},
	
		{"$clr.specular", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleSpecularColor(mat, key, data); }},

		{"$mat.roughnessFactor", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleRoughnessFactor(mat, key, data); }},

		{"$mat.metallicFactor", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleMetallicFactor(mat, key, data); }},

		{"$mat.shininess", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleShininess(mat, key, data); }},

		{"$clr.emissive", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleEmissiveColor(mat, key, data); }},

		{"$clr.transparent", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleTransparentColor(mat, key, data); }},

		{"$mat.opacity", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleOpacity(mat, key, data); }},

		{"$mat.twosided", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleTwoSided(mat, key, data); }},

		{"$mat.gltf.alphaMode", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleAlphaMode(mat, key, data); }},

		{"$mat.gltf.alphaCutoff", [&](aiMaterial* mat, const std::string& key, AssimpMaterialData& data) {
		AssimpLoader::HandleAlphaCutoff(mat, key, data); }},
	};

	for (size_t i = 0; i < material->mNumProperties; i++)
	{
		std::string matKey = material->mProperties[i]->mKey.C_Str();

		// Calls the function that corresponds to the material key.
		auto it = materialKeyToFunction.find(matKey);
		if (it != materialKeyToFunction.end())
		{
			(it->second)(material, matKey, matData);
		}
	}

	LOG_INFO("-------------------------");
}

void AssimpLoader::HandleMaterialName(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.name = GetProperty<aiString>(matKey, material).C_Str();
	//LOG_INFO("Material Name: %s", matData.name.c_str()); redudant, LoadUserMaterial() already prints the name.
}

void AssimpLoader::HandleBumpscaling(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.bumpScale = GetProperty<float>(matKey, material);
	LOG_INFO("Material bumpscale: %f", matData.bumpScale);
}

void AssimpLoader::HandleBlendMode(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData) {
	matData.blendMode = GetProperty<int>(matKey, material);
	LOG_INFO("Material Blend Mode: %i", matData.blendMode);
}

void AssimpLoader::HandleAmbient(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData) {
	AssimpToDirectX(matData.ambient, GetProperty<aiColor3D>(matKey, material));
	LOG_INFO("Material Ambient Color: %f, %f, %f", 
		matData.ambient.x, 
		matData.ambient.y,
		matData.ambient.z);
}

void AssimpLoader::HandleTransparencyFactor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData) {
	matData.alphaCutoff = GetProperty<float>(matKey, material);
	LOG_INFO("Material Transparencyfactor: %f", matData.alphaCutoff);
}

void AssimpLoader::HandleShadingModel(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData) {
	matData.shadingModel = GetProperty<int>(matKey, material);
	LOG_INFO("Material Shadingmodel: %i", matData.shadingModel);
}

void AssimpLoader::HandleDiffuseColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData, MeshFlags::Flags flags)
{
	AssimpToDirectX(matData.albedo, GetProperty<aiColor3D>(matKey, material));
	LOG_INFO("Material Diffuse Color: %f, %f, %f",
		matData.albedo.x,
		matData.albedo.y,
		matData.albedo.z);
}

void AssimpLoader::HandleSpecularColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	AssimpToDirectX(matData.specular, GetProperty<aiColor3D>(matKey, material));
	LOG_INFO("Material Specular Color: %f, %f, %f",
		matData.specular.x,
		matData.specular.y,
		matData.specular.z);
}

void AssimpLoader::HandleRoughnessFactor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.roughness = GetProperty<float>(matKey, material);
	LOG_INFO("Material Roughnessfactor: %f", matData.roughness);
}

void AssimpLoader::HandleMetallicFactor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.metallic = GetProperty<float>(matKey, material);
	LOG_INFO("Material Metallicfactor: %f", matData.metallic);
}

void AssimpLoader::HandleShininess(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.shininess = GetProperty<float>(matKey, material);
	LOG_INFO("Material Shininess: %f", matData.shininess);
}

void AssimpLoader::HandleEmissiveColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	AssimpToDirectX(matData.emissive, GetProperty<aiColor3D>(matKey, material));
	LOG_INFO("Material Emissive Color: %f, %f, %f",
		matData.emissive.x,
		matData.emissive.y,
		matData.emissive.z);
}

void AssimpLoader::HandleTransparentColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	AssimpToDirectX(matData.transparent, GetProperty<aiColor3D>(matKey, material));
	LOG_INFO("Material Transparent Color: %f, %f, %f",
		matData.transparent.x,
		matData.transparent.y,
		matData.transparent.z);
}

void AssimpLoader::HandleOpacity(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.opacity = GetProperty<float>(matKey, material);
	LOG_INFO("Material opacity: %f", matData.opacity);
}

void AssimpLoader::HandleTwoSided(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.bTwoSided = GetProperty<bool>(matKey, material);
	LOG_INFO("Material twosided: %i", matData.bTwoSided);
}

void AssimpLoader::HandleAlphaMode(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.alphaMode = GetProperty<int>(matKey, material);
	LOG_INFO("Material opacity: %i", matData.alphaMode);
}

void AssimpLoader::HandleAlphaCutoff(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData)
{
	matData.alphaCutoff = GetProperty<float>(matKey, material);
	LOG_INFO("Material alphaCutoff: %f", matData.alphaCutoff);
}
