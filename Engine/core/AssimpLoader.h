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

/*
 Originally writted by me in Floof Engine
 My initial plans was to work with the floof team, but I had to change due to compulsory conflics.
*/
class AssimpLoader
{
public:
	AssimpLoader(const std::string& path);
private:
	
	std::string path = "";

	unsigned int aiFlags = aiProcess_CalcTangentSpace 
		| aiProcess_Triangulate 
		| aiProcess_GenUVCoords 
		| aiProcess_FlipUVs;

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

	struct AssimpMesh
	{
		AssimpMaterial material;
		VertexCollection32 vertices;
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