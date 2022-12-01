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

class AssimpLoader
{
public:
	AssimpLoader(const std::string& path);
private:
	
	std::string path = "";

	unsigned int aiFlags = aiProcess_CalcTangentSpace
		| aiProcess_Triangulate
		| aiProcess_GenUVCoords
		| aiProcess_FlipUVs
		| aiProcess_GenSmoothNormals
		| aiProcess_FlipWindingOrder
		| aiProcess_OptimizeMeshes;

	struct AssimpTexture
	{
		std::string path;
		bool IsValid() const
		{
			return path.size() > 3;
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

		float roughness = 0.5f;
		float metallic = 0.5f;
		float shininess = 1.f;
		float opacity = 1.f;

		bool bHasMaterial = false;
	};

	struct AssimpMesh
	{
		std::string name;
		AssimpMaterial material;
		AssimpMaterialData materialData;
		VertexCollection vertices;
		IndexCollection32 indices;
	};

	struct AssimpCollisionMesh
	{
		AssimpMesh mesh;
	} collisionMesh;

	struct AssimpStaticMesh
	{
		std::vector<AssimpMesh> meshes;
	} staticMesh;

	void ProcessNode(aiNode* node, const aiScene* scene);
	bool LoadMesh(aiMesh* mesh, const aiScene* scene);
	void LoadModel(const std::string& path);

public:
	const AssimpStaticMesh& GetAssimpStaticMesh() const
	{
		return staticMesh;
	}
};