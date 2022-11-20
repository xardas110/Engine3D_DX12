#include <EnginePCH.h>
#include <Mesh.h>

#include <Application.h>
#include <ResourceStateTracker.h>
#include <CommandList.h>
#include <CommandQueue.h>

using namespace DirectX;
using namespace Microsoft::WRL;

const D3D12_INPUT_ELEMENT_DESC VertexPositionNormalTexture::InputElements[] =
{
    { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "BITANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

void CreateTangentAndBiTangent(VertexCollection& vertices, IndexCollection32& indices)
{
    for (size_t j = 0; j < indices.size(); j += 3)
    {
        auto i0 = indices[j + 0];
        auto i1 = indices[j + 1];
        auto i2 = indices[j + 2];

        auto& v0 = XMLoadFloat3(&vertices[i0].position);
        auto& v1 = XMLoadFloat3(&vertices[i1].position);
        auto& v2 = XMLoadFloat3(&vertices[i2].position);

        // Shortcuts for UVs
        auto& uv0 = XMLoadFloat2(&vertices[i0].textureCoordinate);
        auto& uv1 = XMLoadFloat2(&vertices[i1].textureCoordinate);
        auto& uv2 = XMLoadFloat2(&vertices[i2].textureCoordinate);

        // Edges of the triangle : position delta
        auto deltaPos1 = XMVector3Normalize(v1 - v0);
        auto deltaPos2 = XMVector3Normalize(v2 - v0);

        // UV delta
        auto deltaUV1 = XMVector2Normalize(uv1 - uv0);
        auto deltaUV2 = XMVector2Normalize(uv2 - uv0);

        float r = 1.0f / (deltaUV1.m128_f32[0] * deltaUV2.m128_f32[1] - deltaUV1.m128_f32[1] * deltaUV2.m128_f32[0]);
        auto tangent = XMVector3Normalize((deltaPos1 * deltaUV2.m128_f32[1] - deltaPos2 * deltaUV1.m128_f32[1]) * r);
        auto bitangent = XMVector3Normalize((deltaPos2 * deltaUV1.m128_f32[0] - deltaPos1 * deltaUV2.m128_f32[0]) * r);

        XMStoreFloat3(&vertices[i0].tangent, tangent);
        XMStoreFloat3(&vertices[i1].tangent, tangent);
        XMStoreFloat3(&vertices[i2].tangent, tangent);

        XMStoreFloat3(&vertices[i0].bitTangent, bitangent);
        XMStoreFloat3(&vertices[i1].bitTangent, bitangent);
        XMStoreFloat3(&vertices[i2].bitTangent, bitangent);
    }
}

MeshInstance::MeshInstance(const std::wstring& path)
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    meshManager.CreateMeshInstance(path, *this);
}

void MeshInstance::SetMaterialInstance(const MaterialInstance& matInstance)
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    meshManager.instanceData.meshInfo[id].materialInstanceID = matInstance.GetMaterialInstanceID();
}

const Texture* MeshInstance::GetTexture(MaterialType::Type type)
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    auto& textureManager = Application::Get().GetAssetManager()->m_TextureManager;
    auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

    auto matInstanceID = meshManager.instanceData.meshInfo[id].materialInstanceID;
    
    auto textureID = materialManager.GetTextureID(type, matInstanceID);
    return textureManager.GetTexture(textureID);
}

const std::wstring& MeshInstance::GetMaterialName()
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
    auto matInstanceID = meshManager.instanceData.meshInfo[id].materialInstanceID;

    return materialManager.GetMaterialInstanceName(matInstanceID);
}

const Material& MeshInstance::GetUserMaterial()
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
    auto matInstanceID = meshManager.instanceData.meshInfo[id].materialInstanceID;

    return materialManager.GetUserDefinedMaterial(matInstanceID);
}

Mesh::Mesh()
    : m_IndexCount(0)
{}

Mesh::~Mesh()
{
    // Allocated resources will be cleaned automatically when the pointers go out of scope.
}

void Mesh::Draw(CommandList& commandList)
{
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.SetVertexBuffer(0, m_VertexBuffer);
    commandList.SetIndexBuffer(m_IndexBuffer);
    commandList.DrawIndexed(m_IndexCount);
}

std::unique_ptr<Mesh> Mesh::CreateSphere(CommandList& commandList, float diameter, size_t tessellation, bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection32 indices;

    if (tessellation < 3)
        throw std::out_of_range("tessellation parameter out of range");

    float radius = diameter / 2.0f;
    size_t verticalSegments = tessellation;
    size_t horizontalSegments = tessellation * 2;

    // Create rings of vertices at progressively higher latitudes.
    for (size_t i = 0; i <= verticalSegments; i++)
    {
        float v = 1 - (float)i / verticalSegments;

        float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
        float dy, dxz;

        XMScalarSinCos(&dy, &dxz, latitude);

        // Create a single ring of vertices at this latitude.
        for (size_t j = 0; j <= horizontalSegments; j++)
        {
            float u = (float)j / horizontalSegments;

            float longitude = j * XM_2PI / horizontalSegments;
            float dx, dz;

            XMScalarSinCos(&dx, &dz, longitude);

            dx *= dxz;
            dz *= dxz;

            XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);
            XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

            vertices.push_back(VertexPositionNormalTexture(normal * radius, normal, textureCoordinate));
        }
    }

    // Fill the index buffer with triangles joining each pair of latitude rings.
    size_t stride = horizontalSegments + 1;

    for (size_t i = 0; i < verticalSegments; i++)
    {
        for (size_t j = 0; j <= horizontalSegments; j++)
        {
            size_t nextI = i + 1;
            size_t nextJ = (j + 1) % stride;

            indices.push_back(( i * stride + j ));
            indices.push_back((nextI * stride + j));
            indices.push_back((i * stride + nextJ));

            indices.push_back((i * stride + nextJ));
            indices.push_back((nextI * stride + j));
            indices.push_back((nextI * stride + nextJ));
        }
    }

    CreateTangentAndBiTangent(vertices, indices);

    // Create the primitive object.
    std::unique_ptr<Mesh> mesh(new Mesh());

    mesh->Initialize(commandList, vertices, indices, rhcoords);

    return mesh;
}

std::unique_ptr<Mesh> Mesh::CreateCube(CommandList& commandList, float size, bool rhcoords)
{
    // A cube has six faces, each one pointing in a different direction.
    const int FaceCount = 6;

    static const XMVECTORF32 faceNormals[FaceCount] =
    {
        { 0,  0,  1 },
        { 0,  0, -1 },
        { 1,  0,  0 },
        { -1,  0,  0 },
        { 0,  1,  0 },
        { 0, -1,  0 },
    };

    static const XMVECTORF32 textureCoordinates[4] =
    {
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 },
    };

    VertexCollection vertices;
    IndexCollection32 indices;

    size /= 2;

    // Create each face in turn.
    for (int i = 0; i < FaceCount; i++)
    {
        XMVECTOR normal = faceNormals[i];

        // Get two vectors perpendicular both to the face normal and to each other.
        XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

        XMVECTOR side1 = XMVector3Cross(normal, basis);
        XMVECTOR side2 = XMVector3Cross(normal, side1);

        // Six indices (two triangles) per face.
        size_t vbase = vertices.size();
        indices.push_back(static_cast<uint16_t>(vbase + 0));
        indices.push_back(static_cast<uint16_t>(vbase + 1));
        indices.push_back(static_cast<uint16_t>(vbase + 2));

        indices.push_back(static_cast<uint16_t>(vbase + 0));
        indices.push_back(static_cast<uint16_t>(vbase + 2));
        indices.push_back(static_cast<uint16_t>(vbase + 3));

        // Four vertices per face.
        vertices.push_back(VertexPositionNormalTexture((normal - side1 - side2) * size, normal, textureCoordinates[0]));
        vertices.push_back(VertexPositionNormalTexture((normal - side1 + side2) * size, normal, textureCoordinates[1]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 + side2) * size, normal, textureCoordinates[2]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 - side2) * size, normal, textureCoordinates[3]));
    }

    CreateTangentAndBiTangent(vertices, indices);

    // Create the primitive object.
    std::unique_ptr<Mesh> mesh(new Mesh());

    mesh->Initialize(commandList, vertices, indices, rhcoords);

    return mesh;
}

// Helper computes a point on a unit circle, aligned to the x/z plane and centered on the origin.
static inline XMVECTOR GetCircleVector(size_t i, size_t tessellation)
{
    float angle = i * XM_2PI / tessellation;
    float dx, dz;

    XMScalarSinCos(&dx, &dz, angle);

    XMVECTORF32 v = { dx, 0, dz, 0 };
    return v;
}

static inline XMVECTOR GetCircleTangent(size_t i, size_t tessellation)
{
    float angle = (i * XM_2PI / tessellation) + XM_PIDIV2;
    float dx, dz;

    XMScalarSinCos(&dx, &dz, angle);

    XMVECTORF32 v = { dx, 0, dz, 0 };
    return v;
}

// Helper creates a triangle fan to close the end of a cylinder / cone
static void CreateCylinderCap(VertexCollection& vertices, IndexCollection32& indices, size_t tessellation, float height, float radius, bool isTop)
{
    // Create cap indices.
    for (size_t i = 0; i < tessellation - 2; i++)
    {
        size_t i1 = (i + 1) % tessellation;
        size_t i2 = (i + 2) % tessellation;

        if (isTop)
        {
            std::swap(i1, i2);
        }

        size_t vbase = vertices.size();
        indices.push_back(static_cast<uint16_t>(vbase));
        indices.push_back(static_cast<uint16_t>(vbase + i1));
        indices.push_back(static_cast<uint16_t>(vbase + i2));
    }

    // Which end of the cylinder is this?
    XMVECTOR normal = g_XMIdentityR1;
    XMVECTOR textureScale = g_XMNegativeOneHalf;

    if (!isTop)
    {
        normal = -normal;
        textureScale *= g_XMNegateX;
    }

    // Create cap vertices.
    for (size_t i = 0; i < tessellation; i++)
    {
        XMVECTOR circleVector = GetCircleVector(i, tessellation);

        XMVECTOR position = (circleVector * radius) + (normal * height);

        XMVECTOR textureCoordinate = XMVectorMultiplyAdd(XMVectorSwizzle<0, 2, 3, 3>(circleVector), textureScale, g_XMOneHalf);

        vertices.push_back(VertexPositionNormalTexture(position, normal, textureCoordinate));
    }
}

std::unique_ptr<Mesh> Mesh::CreateCone(CommandList& commandList, float diameter, float height, size_t tessellation, bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection32 indices;

    if (tessellation < 3)
        throw std::out_of_range("tessellation parameter out of range");

    height /= 2;

    XMVECTOR topOffset = g_XMIdentityR1 * height;

    float radius = diameter / 2;
    size_t stride = tessellation + 1;

    // Create a ring of triangles around the outside of the cone.
    for (size_t i = 0; i <= tessellation; i++)
    {
        XMVECTOR circlevec = GetCircleVector(i, tessellation);

        XMVECTOR sideOffset = circlevec * radius;

        float u = (float)i / tessellation;

        XMVECTOR textureCoordinate = XMLoadFloat(&u);

        XMVECTOR pt = sideOffset - topOffset;

        XMVECTOR normal = XMVector3Cross(GetCircleTangent(i, tessellation), topOffset - pt);
        normal = XMVector3Normalize(normal);

        // Duplicate the top vertex for distinct normals
        vertices.push_back(VertexPositionNormalTexture(topOffset, normal, g_XMZero));
        vertices.push_back(VertexPositionNormalTexture(pt, normal, textureCoordinate + g_XMIdentityR1));

        indices.push_back(static_cast<uint16_t>(i * 2));
        indices.push_back(static_cast<uint16_t>((i * 2 + 3) % (stride * 2)));
        indices.push_back(static_cast<uint16_t>((i * 2 + 1) % (stride * 2)));
    }

    // Create flat triangle fan caps to seal the bottom.
    CreateCylinderCap(vertices, indices, tessellation, height, radius, false);

    CreateTangentAndBiTangent(vertices, indices);

    // Create the primitive object.
    std::unique_ptr<Mesh> mesh(new Mesh());

    mesh->Initialize(commandList, vertices, indices, rhcoords);

    return mesh;
}

std::unique_ptr<Mesh> Mesh::CreateTorus(CommandList& commandList, float diameter, float thickness, size_t tessellation, bool rhcoords)
{
    VertexCollection vertices;
    IndexCollection32 indices;

    if (tessellation < 3)
        throw std::out_of_range("tessellation parameter out of range");

    size_t stride = tessellation + 1;

    // First we loop around the main ring of the torus.
    for (size_t i = 0; i <= tessellation; i++)
    {
        float u = (float)i / tessellation;

        float outerAngle = i * XM_2PI / tessellation - XM_PIDIV2;

        // Create a transform matrix that will align geometry to
        // slice perpendicularly though the current ring position.
        XMMATRIX transform = XMMatrixTranslation(diameter / 2, 0, 0) * XMMatrixRotationY(outerAngle);

        // Now we loop along the other axis, around the side of the tube.
        for (size_t j = 0; j <= tessellation; j++)
        {
            float v = 1 - (float)j / tessellation;

            float innerAngle = j * XM_2PI / tessellation + XM_PI;
            float dx, dy;

            XMScalarSinCos(&dy, &dx, innerAngle);

            // Create a vertex.
            XMVECTOR normal = XMVectorSet(dx, dy, 0, 0);
            XMVECTOR position = normal * thickness / 2;
            XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

            position = XMVector3Transform(position, transform);
            normal = XMVector3TransformNormal(normal, transform);

            vertices.push_back(VertexPositionNormalTexture(position, normal, textureCoordinate));

            // And create indices for two triangles.
            size_t nextI = (i + 1) % stride;
            size_t nextJ = (j + 1) % stride;

            indices.push_back(static_cast<uint16_t>(i * stride + j));
            indices.push_back(static_cast<uint16_t>(i * stride + nextJ));
            indices.push_back(static_cast<uint16_t>(nextI * stride + j));

            indices.push_back(static_cast<uint16_t>(i * stride + nextJ));
            indices.push_back(static_cast<uint16_t>(nextI * stride + nextJ));
            indices.push_back(static_cast<uint16_t>(nextI * stride + j));
        }
    }

    CreateTangentAndBiTangent(vertices, indices);

    // Create the primitive object.
    std::unique_ptr<Mesh> mesh(new Mesh());

    mesh->Initialize(commandList, vertices, indices, rhcoords);

    return mesh;
}

std::unique_ptr<Mesh> Mesh::CreatePlane(CommandList& commandList, float width, float height, bool rhcoords)
{
    VertexCollection vertices = 
    {
        { XMFLOAT3(-0.5f * width, 0.0f,  0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 0
        { XMFLOAT3( 0.5f * width, 0.0f,  0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 1
        { XMFLOAT3( 0.5f * width, 0.0f, -0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 2
        { XMFLOAT3(-0.5f * width, 0.0f, -0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }  // 3
    };
    
    IndexCollection32 indices =
    {
        0, 3, 1, 1, 3, 2
    };

    CreateTangentAndBiTangent(vertices, indices);

    std::unique_ptr<Mesh> mesh(new Mesh());

    mesh->Initialize(commandList, vertices, indices, rhcoords);

    return mesh;
}

std::unique_ptr<Mesh> Mesh::CreateMesh(CommandList& commandList, VertexCollection& vertices, IndexCollection32& indices, bool rhcoords)
{
    std::unique_ptr<Mesh> mesh(new Mesh());
    mesh->Initialize(commandList, vertices, indices, rhcoords);
    return mesh;
}

// Helper for flipping winding of geometric primitives for LH vs. RH coords
static void ReverseWinding(IndexCollection32& indices, VertexCollection& vertices)
{
    assert((indices.size() % 3) == 0);
    for (auto it = indices.begin(); it != indices.end(); it += 3)
    {
        std::swap(*it, *(it + 2));
    }

    for (auto it = vertices.begin(); it != vertices.end(); ++it)
    {
        it->textureCoordinate.x = (1.f - it->textureCoordinate.x);
    }
}

void Mesh::Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection32& indices, bool rhcoords)
{
    if (vertices.size() >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    if (!rhcoords)
        ReverseWinding(indices, vertices);

    commandList.CopyVertexBuffer(m_VertexBuffer, vertices);
    commandList.CopyIndexBuffer(m_IndexBuffer, indices);

    m_IndexCount = static_cast<UINT>(indices.size());
}

void Mesh::InitializeBlas(CommandList& commandList)
{
    auto device = Application::Get().GetDevice();

    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
    desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = m_IndexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = m_IndexCount;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = m_VertexBuffer.GetNumVertices();
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_VertexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexPositionNormalTexture);
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
    blasDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    blasDesc.Inputs.NumDescs = 1;
    blasDesc.Inputs.pGeometryDescs = &geometryDesc;
    blasDesc.Inputs.Flags = buildFlag;
    blasDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo;
    device->GetRaytracingAccelerationStructurePrebuildInfo(&blasDesc.Inputs, &bottomLevelPrebuildInfo);

    auto blasBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &blasBufferDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&scratchResource));

    scratchResource->SetName(L"Scratch resource");

    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &blasBufferDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(&blas));

    blasDesc.DestAccelerationStructureData = blas->GetGPUVirtualAddress();
    blasDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();

    blas->SetName(L"Blas");

    commandList.GetGraphicsCommandList()->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
}

StaticMesh::StaticMesh(const std::string& path)
{
   // auto* smm = Application::Get().GetAssetManager();
    //smm->LoadStaticMesh(path, *this);
}
