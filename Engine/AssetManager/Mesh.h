#pragma once
/*
 *  Copyright(c) 2018 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

/**
 *  @file Mesh.h
 *  @date October 24, 2018
 *  @author Jeremiah van Oosten
 *
 *  @brief A mesh class encapsulates the index and vertex buffers for a geometric primitive.
 */

#include <CommandList.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>

#include <DirectXMath.h>
#include <d3d12.h>

#include <wrl.h>

#include <memory> // For std::unique_ptr
#include <vector>
#include <Material.h>
#include <TypesCompat.h>
#include <Helpers.h>
#include <Transform.h>
#include <Vertex.h>

using MeshID = UINT;
using MeshInstanceID = UINT;

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

namespace Primitives
{
    enum Type
    {
        Cube,
        Size
    };
}

struct MeshInstance
{
    friend class DeferredRenderer;
    friend class MeshManager;
    friend class Raytracing;
    friend class Editor;
    friend class StaticMeshInstance;

    //Path or name
    MeshInstance(const std::wstring& path);
    void SetMaterialInstance(const MaterialInstance& matInstance);

    const Texture* GetTexture(MaterialType::Type type);

    const std::wstring& GetName();

    const std::wstring& GetMaterialName();
    const MaterialColor& GetMaterialColor() const;

    MeshInstanceID GetInstanceID() const
    {
        return id;
    }

    bool IsValid() const
    {
        return id != UINT_MAX;
    }

    bool HasOpacity()
    {
        if (GetTexture(MaterialType::opacity)) return true;

        auto& mat = GetMaterialColor();
        
        if (mat.diffuse.w >= 1.f) return false;

        return true;
    }

    bool IsPointlight();

private:
    MeshInstance() = default;
    MeshInstance(MeshInstanceID id) :id(id) {};
    MeshInstanceID id{UINT_MAX};
};

//Internal mesh
class Mesh
{
    friend class DeferredRenderer;
    friend class MeshManager;
    friend class Raytracing;
public:

    Mesh(Mesh&& move) = default;
    Mesh& operator=(Mesh&&) = default;

    virtual ~Mesh();

    void Draw(CommandList& commandList);

    static std::unique_ptr<Mesh> CreateCube(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float size = 1, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreateSphere(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float diameter = 1, size_t tessellation = 16, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreateCone(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreateTorus(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = false);

    static std::unique_ptr<Mesh> CreatePlane(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, float width = 1, float height = 1, bool rhcoords = false);

    static Mesh CreateMesh(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, VertexCollection& vertices, IndexCollection32& indices, bool rhcoords, bool calcTangent);

private:
    friend class AssetManager;
    friend struct std::default_delete<Mesh>;

    Mesh();
    Mesh(const Mesh& copy) = delete;
    
    void Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection32& indices, bool rhcoords);
    void InitializeBlas(CommandList& commandList);

    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> blas;
    Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource;

    UINT m_IndexCount;
};

class StaticMeshInstance
{
    friend class AssetManager;
    friend class DeferredRenderer;
    friend class MeshManager;
    friend class Editor;

public:
    StaticMeshInstance() = default;
    StaticMeshInstance(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, 
        const std::string& path, MeshFlags::Flags flags = MeshFlags::None);

private:
    std::uint32_t startOffset{0U};
    std::uint32_t endOffset{0U};
};

struct MeshInstanceWrapper
{
    MeshInstanceWrapper(Transform& trans, MeshInstance instance)
        :trans(trans), instance(instance) {}
        
    Transform& trans; //complete world transform
    MeshInstance instance;    
};