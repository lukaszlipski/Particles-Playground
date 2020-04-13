#pragma once

using VertexFormatDesc = std::vector<D3D12_INPUT_ELEMENT_DESC>;
using VertexFormatDescRef = VertexFormatDesc*;

// None
struct None {};

static VertexFormatDesc NoneVertexFormatDesc = { {
} };

// Default Vertex
struct DefaultVertex
{
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
};

static VertexFormatDesc DefaultVertexFormatDesc = { {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DefaultVertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DefaultVertex, TexCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
} };

template<typename T>
VertexFormatDescRef GetVertexFormatDesc()
{
    if constexpr (std::is_same_v<T, DefaultVertex>)
    {
        return &DefaultVertexFormatDesc;
    }
    else if constexpr (std::is_same_v<T, None>)
    {
        return &NoneVertexFormatDesc;
    }
    else
    {
        static_assert(false, "Cannot find VertexFormatDesc for given Vertex structure");
        return nullptr;
    }
}
