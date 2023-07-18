#pragma once
#include <Mesh.h>
#include <filesystem>

namespace AssimpMaterialType
{
	enum Type
	{
		Ambient,
		Albedo,
		Diffuse = Albedo,
		Specular,
		Height,
		Normal = Height,
		Roughness,
		Metallic,
		Opacity,
		Emissive,
		Size
	};
}

struct AssimpTexture
{
	AssimpTexture::AssimpTexture() = default;

	AssimpTexture::AssimpTexture(const std::string& path)
		: path(path) {}
	
	// Copy Constructor
	AssimpTexture::AssimpTexture(const AssimpTexture& other) = default;

	// Move Constructor
	AssimpTexture::AssimpTexture(AssimpTexture&& other) = delete;

	// Copy Assignment Operator
	AssimpTexture& AssimpTexture::operator=(const AssimpTexture& other) = delete;

	// Move Assignment Operator
	AssimpTexture& AssimpTexture::operator=(AssimpTexture&& other) = delete;

	bool IsValid() const
	{
		std::filesystem::path fsPath(path);
		return std::filesystem::exists(fsPath);
	}

	std::string path;
};

struct AssimpMaterial
{
	bool HasTexture(AssimpMaterialType::Type type) const
	{
		return textures[type].IsValid();
	}

	const AssimpTexture& GetTexture(AssimpMaterialType::Type type) const
	{
		return textures[type];
	}

	AssimpTexture textures[AssimpMaterialType::Size];
};

struct AssimpMaterialData
{
	std::string name;
	int shadingModel{};
	DirectX::XMFLOAT3 albedo{1.f, 1.f, 1.f};
	DirectX::XMFLOAT3 specular{ 1.f, 1.f, 1.f };
	DirectX::XMFLOAT3 emissive{ 0.f, 0.f, 0.f };
	DirectX::XMFLOAT3 transparent{ 1.f, 1.f, 1.f };
	DirectX::XMFLOAT3 ambient{ 1.f, 1.f, 1.f };

	float roughness = 1.0f;
	float metallic = 1.0f;
	float shininess = 1.f;
	float opacity = 1.f;

	float alphaCutoff = 1.f;
	int alphaMode = 0; //opaque, blend
	int blendMode = 0;

	float bumpScale = 1.f;

	bool bHasMaterial = false;
	bool bTwoSided = false;
	bool bHasRoughness = false;
};

struct AssimpMesh
{
	std::string name;
	AssimpMaterial material;
	AssimpMaterialData materialData;
	VertexCollection vertices;
	IndexCollection32 indices;
};

struct AssimpStaticMesh
{
	// Default constructor
	AssimpStaticMesh() : meshes() {}

	// Copy constructor (deleted)
	AssimpStaticMesh(const AssimpStaticMesh&) = delete;

	// Move constructor (deleted)
	AssimpStaticMesh(AssimpStaticMesh&&) = delete;

	// Copy assignment operator (deleted)
	AssimpStaticMesh& operator=(const AssimpStaticMesh&) = delete;

	// Move assignment operator (deleted)
	AssimpStaticMesh& operator=(AssimpStaticMesh&&) = delete;

	std::vector<AssimpMesh> meshes;
};

class AssimpLoader
{
public:
	// Default constructor 
	AssimpLoader() = delete;

	AssimpLoader(const std::string& path, MeshImport::Flags flags);

	// Move constructor 
	AssimpLoader(AssimpLoader&&) = default;

	// Copy constructor 
	AssimpLoader(const AssimpLoader&) = delete;

	// Copy assignment operator
	AssimpLoader& operator=(const AssimpLoader& other) = delete;

	// Move assignment operator
	AssimpLoader& operator=(AssimpLoader&& other) = default;

	const AssimpStaticMesh& GetAssimpStaticMesh() const;

private:
	void ProcessNode(aiNode* node, const aiScene* scene, MeshImport::Flags flags);

	// Loads a mesh from Assimp and stores the data into our custom Mesh class.
	bool LoadMesh(aiMesh* mesh, const aiScene* scene, MeshImport::Flags flags);

	void LoadModel(const std::string& path, MeshImport::Flags flags);

	// Validates that the path exists
	bool ValidatePath(const std::string& path);

	// Prepares the flag copy for Assimp
	unsigned int PrepareFlags(MeshImport::Flags flags);

	// Loads the scene from the file using Assimp
	const aiScene* LoadScene(Assimp::Importer& importer, const std::string& path, unsigned int flags);

	// Validates the loaded scene
	bool ValidateScene(const aiScene* scene, const Assimp::Importer& importer);

	// Sets the name of the internal mesh.
	void SetMeshName(AssimpMesh& internalMesh, aiMesh* mesh);

	// Loads vertices, normals, texture coordinates and tangents of the mesh.
	void LoadVertices(AssimpMesh& internalMesh, aiMesh* mesh, MeshImport::Flags flags);

	// Sets position of a vertex.
	void SetVertexPosition(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i);

	// Sets normal of a vertex.
	void SetVertexNormal(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i, MeshImport::Flags flags);

	// Sets texture coordinates of a vertex.
	void SetVertexTextureCoords(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i);

	// Sets tangents and bitangents of a vertex.
	void SetVertexTangents(VertexPositionNormalTexture& vertex, aiMesh* mesh, unsigned int i);

	// Loads indices of the mesh.
	void LoadIndices(AssimpMesh& internalMesh, aiMesh* mesh);

	// Checks if the mesh has bones and prints a warning message if it does.
	void CheckForBones(aiMesh* mesh);

	// Loads materials of the mesh.
	void LoadMaterials(AssimpMesh& internalMesh, aiMesh* mesh, const aiScene* scene, MeshImport::Flags flags);

	// Loads textures associated with a material.
	void LoadTextures(AssimpMesh& internalMesh, aiMaterial* material, MeshImport::Flags flags);

	// Loads user material data from Assimp material.
   // This method iterates over material properties and processes each one based on its key.
	void LoadUserMaterial(aiMaterial* material, AssimpMaterialData& matData, MeshImport::Flags flags);

	//Handle functions for all the material properties
	void HandleMaterialName(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleBumpscaling(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleBlendMode(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleAmbient(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleTransparencyFactor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleShadingModel(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleDiffuseColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData, MeshImport::Flags flags);
	void HandleSpecularColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleRoughnessFactor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleMetallicFactor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleShininess(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleEmissiveColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleTransparentColor(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleOpacity(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleTwoSided(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleAlphaMode(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);
	void HandleAlphaCutoff(aiMaterial* material, const std::string& matKey, AssimpMaterialData& matData);

	// Template function to get a property value from an Assimp material based on its key.
	// The type of the property value is determined by the template argument.
	template <typename T>
	T GetProperty(const std::string& key, aiMaterial* material)
	{
		T value;
		material->Get(key.c_str(), 0, 0, value);

		return value;
	}

	unsigned int aiFlags = aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords;

	std::string path = "";

	AssimpStaticMesh staticMesh;
};