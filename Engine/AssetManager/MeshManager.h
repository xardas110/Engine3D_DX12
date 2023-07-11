#pragma once

/**
 *  @file MeshManager.h
 *  @date October 20, 2022
 *  @author Xardas110
 *
 *  @brief The MeshManager, based on the factory pattern is a struct which manages all the meshes for the application.
 */

#include <Mesh.h>
#include <eventcpp/event.hpp>
#include <TypesCompat.h>

class SRVHeapData;

struct MeshManager
{
	friend class AssetManager;
	friend class MeshInstance;
	friend class Raytracing;
	friend class DeferredRenderer;
	friend class Editor;
	friend class StaticMeshInstance;

private:

	MeshManager(const SRVHeapData& srvHeapData);

	bool CreateMeshInstance(const std::wstring& path, MeshInstance& outMeshInstanceID);

	void LoadStaticMesh(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, const std::string& path, StaticMeshInstance& outStaticMesh, MeshImport::Flags flags = MeshImport::None);

	//Per component data
	struct InstanceData
	{
		friend class MeshManager;
		friend class MeshInstance;
		friend class Raytracing;
		friend class DeferredRenderer;
		friend class Editor;
		friend class StaticMeshInstance;
	private:

		MeshInstanceID CreateInstance(MeshID meshID, const MeshInfo& meshInfo);

		std::vector<MeshID> meshIds;
		std::vector<MeshInfo> meshInfo; //flags and material can change per instance
	} instanceData; //instance id

	/**
	* MeshTuple is a struct which contains information about a mesh for both CPU and GPU side
	*/
	struct MeshTuple
	{
		friend struct MeshManager;
		friend class Raytracing;
		friend class DeferredRenderer;
	private:
		MeshTuple(Mesh&& mesh, MeshInfo meshInfo) : mesh(std::move(mesh)), meshInfo(meshInfo) {};

		// CPU information.
		Mesh mesh;

		// GPU information.
		MeshInfo meshInfo;
	};

	struct MeshData
	{
		friend class MeshManager;
		friend class Raytracing;
		friend class DeferredRenderer;
		friend class Editor;
		friend class MeshInstance;
		friend class StaticMeshInstance;
	public:
		template<typename TClass, typename TRet, typename ...Args>
		void AttachMeshCreatedEvent(TRet(TClass::* func) (Args...), TClass* obj)
		{
			meshCreationEvent.attach(func, *obj);
		}

	private:

		MeshID GetMeshID(const std::wstring& name);
		const std::wstring& GetName(MeshID id);

		void AddMesh(const std::wstring& name, MeshTuple& tuple);

		MeshTuple CreateMesh(const std::wstring& name, Mesh&& mesh, const SRVHeapData& heap);

		std::map<std::wstring, MeshID> map;
		std::vector<MeshTuple> meshes;
		event::event<void(const Mesh&)> meshCreationEvent;
	} meshData;

	const SRVHeapData& m_SrvHeapData;
};