#pragma once

#include <CommandList.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>

#include <DirectXMath.h>
#include <d3d12.h>

#include <wrl.h>

#include <memory>
#include <vector>
#include <Material.h>
#include <TypesCompat.h>
#include <Helpers.h>
#include <Transform.h>
#include <Vertex.h>

#define MESH_INVALID 0xffffffff

class MeshManager;
struct MeshInstance;

using MeshID = std::uint32_t;
using MeshRefCount = std::uint32_t;

namespace MeshFlags
{
    enum Flags
    {
        None = 0,
        HeightAsNormal = 1 << 0,
        IgnoreUserMaterial = 1 << 1,
        IgnoreUserMaterialAlbedoOnly = 1 << 2,
        RHCoords = 1 << 3,
        ConvertShininiessToAlpha = 1 << 4,
        AmbientAsMetallic = 1 << 5,
        ForceAlphaBlend = 1 << 6, //Forces blending mode on all imported materials with opacity
        ForceAlphaCutoff = 1 << 7,
        SkipTextures = 1 << 8,
        BaseColorAlpha = 1 << 9,
        AO_Rough_Metal_As_Spec_Tex = 1 << 10,
        InvertNormals = 1 << 11, //Geometry normal only
        InvertMaterialNormals = 1 << 12,
        AssimpTangent = 1 << 13,
        CustomTangent = 1 << 14,
    };
}

inline MeshFlags::Flags operator|(MeshFlags::Flags a, MeshFlags::Flags b)
{
    return static_cast<MeshFlags::Flags>(static_cast<int>(a) | static_cast<int>(b));
}

class MeshInstanceAccess
{
    friend class DeferredRenderer;
    friend class Raytracing;

    static MeshID GetMeshID(const MeshInstance meshInstance);
};

struct MeshInstance
{
    friend class MeshManager;
    friend class MeshInstanceAccess;

    MeshInstance();
    MeshInstance(const std::wstring& name);

    explicit MeshInstance(const MeshID id);
    MeshInstance(const MeshInstance& meshInstance);
    MeshInstance(MeshInstance&& meshInstance) noexcept;

    MeshInstance(
        const std::wstring& name,
        std::shared_ptr<CommandList> cmdList,
        std::shared_ptr<CommandList> rtCmdList,
        VertexCollection& vertices, IndexCollection32& indices,
        bool rhcoords, bool calcTangent);
        

    MeshInstance& operator= (const MeshInstance& meshInstance);
    MeshInstance& operator= (MeshInstance&& meshInstance) noexcept;

    ~MeshInstance();

    MeshInfo GetGPUInfo() const;

    void SetFlags(UINT flags);
    void AddFlags(UINT flags);

    UINT GetFlags() const;

    bool IsValid() const;
    
    bool IsPointlight();

private:
    MeshID id{ MESH_INVALID };
};

//Internal mesh
class Mesh
{
    friend class DeferredRenderer;
    friend class MeshManager;
    friend class Raytracing;
public:
    Mesh();

    Mesh(const Mesh& copy) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& move) = default;
    Mesh& operator=(Mesh&&) = default;
   
    virtual ~Mesh();

    void Draw(CommandList& commandList) const;

    static std::unique_ptr<Mesh> CreateCube(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float size = 1, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreateSphere(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float diameter = 1, size_t tessellation = 16, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreateCone(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreateTorus(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreatePlane(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float width = 1, float height = 1, bool rhcoords = false);

    static Mesh CreateMesh(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, VertexCollection& vertices, IndexCollection32& indices, bool rhcoords, bool calcTangent);

private:
    friend class AssetManager;
    friend struct std::default_delete<Mesh>;

    void Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection32& indices, bool rhcoords);
    void InitializeBlas(CommandList& commandList);

    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> blas;
    Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource;

    UINT m_IndexCount;
};

struct MeshInstanceWrapper
{
    MeshInstanceWrapper(Transform& trans, MeshInstance instance, MaterialInstance matInstance)
        :trans(trans), instance(instance), matInstance(matInstance) {}

    Transform trans; //complete world transform
    MeshInstance instance;
    MaterialInstance matInstance;
};