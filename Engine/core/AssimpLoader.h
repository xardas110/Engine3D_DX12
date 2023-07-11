#pragma once
#include <Mesh.h>

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
	std::string path;
	bool IsValid() const
	{
		return !path.empty();
	}
};

struct AssimpMaterial
{
	bool HasTexture(AssimpMaterialType::Type type) const
	{
		return textures[type].IsValid();
	}

	AssimpTexture GetTexture(AssimpMaterialType::Type type) const
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

	float roughness = 0.5f;
	float metallic = 0.5f;
	float shininess = 0.f;
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

class AssimpLoader
{
public:
	AssimpLoader(const std::string& path, MeshImport::Flags flags);
private:
	
	std::string path = "";

	unsigned int aiFlags =  aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords;

private:
	struct AssimpCollisionMesh
	{
		AssimpMesh mesh;
	} collisionMesh;

	struct AssimpStaticMesh
	{
		std::vector<AssimpMesh> meshes;
	} staticMesh;

	 //inoutMatData
	void AssimpLoader::LoadUserMaterial(aiMaterial* material, AssimpMaterialData& matData, MeshImport::Flags flags);

	void ProcessNode(aiNode* node, const aiScene* scene, MeshImport::Flags flags);
	bool LoadMesh(aiMesh* mesh, const aiScene* scene, MeshImport::Flags flags);
	void LoadModel(const std::string& path, MeshImport::Flags flags);

public:
	const AssimpStaticMesh& GetAssimpStaticMesh() const
	{
		return staticMesh;
	}
};