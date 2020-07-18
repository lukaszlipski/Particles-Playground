#include "sampler.h"

Sampler::Sampler()
{
    mDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    mDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    mDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    mDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    mDesc.MipLODBias = 0.0f;
    mDesc.MaxAnisotropy = 0;
    mDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    mDesc.BorderColor[0] = 0.0f;
    mDesc.BorderColor[1] = 0.0f;
    mDesc.BorderColor[2] = 0.0f;
    mDesc.BorderColor[3] = 0.0f;
    mDesc.MinLOD = 0.0f;
    mDesc.MaxLOD = D3D12_FLOAT32_MAX;
}

Sampler& Sampler::SetAddressMode(TextureAddressMode mode)
{
    mDesc.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(mode);
    mDesc.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(mode);
    mDesc.AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(mode);

    return *this;
}
