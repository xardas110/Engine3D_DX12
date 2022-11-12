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

 // Vertex struct holding position, normal vector, and texture mapping information.
struct VertexPositionNormalTexture
{
    VertexPositionNormalTexture()
    { }

    VertexPositionNormalTexture(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& textureCoordinate)
        : position(position),
        normal(normal),
        textureCoordinate(textureCoordinate)
    { }

    VertexPositionNormalTexture(DirectX::FXMVECTOR position, DirectX::FXMVECTOR normal, DirectX::FXMVECTOR textureCoordinate)
    {
        XMStoreFloat3(&this->position, position);
        XMStoreFloat3(&this->normal, normal);
        XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
    }

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 textureCoordinate;

    static const int InputElementCount = 3;
    static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

// Vertex struct holding position, normal vector, and texture mapping information.
struct VertexPositionNormalTextureTangentBitangent
{
    VertexPositionNormalTextureTangentBitangent()
    { }

    VertexPositionNormalTextureTangentBitangent(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& textureCoordinate, const DirectX::XMFLOAT3& tangent, const DirectX::XMFLOAT3& bitTangent)
        : position(position),
        normal(normal),
        textureCoordinate(textureCoordinate),
        tangent(tangent),
        bitTangent(bitTangent)
    { }

    VertexPositionNormalTextureTangentBitangent(DirectX::FXMVECTOR position, DirectX::FXMVECTOR normal, DirectX::FXMVECTOR textureCoordinate, DirectX::FXMVECTOR tangent, DirectX::FXMVECTOR bitTangent)
    {
        XMStoreFloat3(&this->position, position);
        XMStoreFloat3(&this->normal, normal);
        XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        XMStoreFloat3(&this->tangent, tangent);
        XMStoreFloat3(&this->bitTangent, bitTangent);
    }

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 textureCoordinate;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT3 bitTangent;

    static const int InputElementCount = 5;
    static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

using VertexCollection = std::vector<VertexPositionNormalTexture>;
using IndexCollection = std::vector<uint16_t>;

using VertexCollection32 = std::vector<VertexPositionNormalTextureTangentBitangent>;
using IndexCollection32 = std::vector<uint32_t>;

namespace Primitives
{
    enum Type
    {
        Cube,
        Size
    };
}

using MeshID = std::uint32_t;
using SRVHeapID = std::uint32_t;

struct MeshWrapper
{

private:
    MeshID meshID = UINT_MAX;
    SRVHeapID srvHeapID = UINT_MAX;
};

class Mesh
{
    friend class DeferredRenderer;
    friend class Raytracing;
public:

    void Draw(CommandList& commandList);

    static std::unique_ptr<Mesh> CreateCube(CommandList& commandList, float size = 1, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreateSphere(CommandList& commandList, float diameter = 1, size_t tessellation = 16, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreateCone(CommandList& commandList, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreateTorus(CommandList& commandList, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreatePlane(CommandList& commandList, float width = 1, float height = 1, bool rhcoords = false);
    //Used to create a mesh from a model
    static std::unique_ptr<Mesh> CreateMesh(CommandList& commandList, const VertexCollection32& vertices, const IndexCollection32& indices, bool rhcoords);

    virtual ~Mesh();

protected:

private:
    friend class AssetManager;
    friend struct std::default_delete<Mesh>;

    Mesh();
    Mesh(const Mesh& copy) = delete;
    
    void Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices, bool rhcoords);

    void Initialize(CommandList& commandList, const VertexCollection32& vertices, const IndexCollection32& indices, bool rhcoords);

    Material m_Material;

    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

    UINT m_IndexCount;
};

class StaticMesh
{
    friend class AssetManager;
    friend class DeferredRenderer;

public:
    StaticMesh() = default;
    StaticMesh(const std::string& path);
private:
    std::uint32_t startOffset{0};
    std::uint32_t count{0};
};