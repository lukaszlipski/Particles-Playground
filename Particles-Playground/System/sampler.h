#pragma once

enum class TextureAddressMode
{
    Wrap = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    Mirror = D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
    Clamp = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    Border = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
    MirrorOnce = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
};

class Sampler 
{
public:
    Sampler();

    Sampler& SetAddressMode(TextureAddressMode mode);

    inline const D3D12_SAMPLER_DESC& GetDesc() const { return mDesc; }
    
private:
    D3D12_SAMPLER_DESC mDesc = {};
};
