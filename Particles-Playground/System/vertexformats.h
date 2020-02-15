#pragma once

struct DefaultVertex
{
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
};

constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 2> DefaultVertexFormatDesc = { {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DefaultVertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, offsetof(DefaultVertex, TexCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
} };

