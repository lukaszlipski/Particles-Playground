#pragma once

using VertexFormatDesc = std::vector<D3D12_INPUT_ELEMENT_DESC>;
using VertexFormatDescRef = VertexFormatDesc*;

template<typename T>
VertexFormatDescRef GetVertexFormatDesc()
{
    static_assert(false, "Cannot find VertexFormatDesc for given Vertex structure");
}

// None
struct None {};

template<>
inline VertexFormatDescRef GetVertexFormatDesc<None>()
{
    static VertexFormatDesc vertexFormat = { { } };
    return &vertexFormat;
}

// Default Vertex
struct DefaultVertex
{
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
};

template<>
inline VertexFormatDescRef GetVertexFormatDesc<DefaultVertex>()
{
    static VertexFormatDesc vertexFormat = { {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DefaultVertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DefaultVertex, TexCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    } };

    return &vertexFormat;
}

