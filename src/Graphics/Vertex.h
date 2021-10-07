#pragma once

#include <Oblivion.h>


enum class VertexType
{
    PositionColorVertex = 0
};
static constexpr const char *VertexTypeString[] =
{
    "PositionColorVertex"
};


struct PositionColorVertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Color;
    PositionColorVertex() = default;
    PositionColorVertex(float x, float y, float z, float r, float g, float b, float a):
        Position(x, y, z), Color(r, g, b, a)
    {
    };
    PositionColorVertex(const DirectX::XMFLOAT3 &position, DirectX::XMFLOAT4 &color):
        Position(position), Color(color)
    {
    };

    static std::array<D3D12_INPUT_ELEMENT_DESC, 2> GetInputElementDesc()
    {
        decltype(GetInputElementDesc()) elements;
        elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        elements[0].InputSlot = 0;
        elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[0].InstanceDataStepRate = 0;
        elements[0].SemanticIndex = 0;
        elements[0].SemanticName = "POSITION";

        elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        elements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        elements[1].InputSlot = 0;
        elements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[1].InstanceDataStepRate = 0;
        elements[1].SemanticIndex = 0;
        elements[1].SemanticName = "COLOR";

        return elements;

    }
    
};


struct PositionNormalTexCoordVertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
    PositionNormalTexCoordVertex() = default;
    PositionNormalTexCoordVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v):
        Position(x, y, z), Normal(nx, ny, nz), TexCoord(u, v)
    {
    };
    PositionNormalTexCoordVertex(const DirectX::XMFLOAT3 &position, DirectX::XMFLOAT3 &normal, const DirectX::XMFLOAT2 &texCoord):
        Position(position), Normal(normal), TexCoord(texCoord)
    {
    };

    static std::array<D3D12_INPUT_ELEMENT_DESC, 3> GetInputElementDesc()
    {
        decltype(GetInputElementDesc()) elements;
        elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        elements[0].InputSlot = 0;
        elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[0].InstanceDataStepRate = 0;
        elements[0].SemanticIndex = 0;
        elements[0].SemanticName = "POSITION";

        elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        elements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        elements[1].InputSlot = 0;
        elements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[1].InstanceDataStepRate = 0;
        elements[1].SemanticIndex = 0;
        elements[1].SemanticName = "NORMAL";

        elements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        elements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        elements[2].InputSlot = 0;
        elements[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elements[2].InstanceDataStepRate = 0;
        elements[2].SemanticIndex = 0;
        elements[2].SemanticName = "TEXCOORD";

        return elements;

    }
};
