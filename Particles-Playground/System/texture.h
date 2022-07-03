#pragma once
#include "resource.h"
#include "gpubufferuploadmanager.h"

class CPUDescriptorHandle;
class CommandList;

using TextureUsageType = uint8_t;

enum class TextureUsage : TextureUsageType
{
    ShaderResource  = 1 << 0,
    RenderTarget    = 1 << 1,
    CopyDst         = 1 << 2,
    CopySrc         = 1 << 3,
    DepthRead       = 1 << 4,
    DepthWrite      = 1 << 5,
    All             = std::numeric_limits<TextureUsageType>::max()
};

constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) {
    return static_cast<TextureUsage>(static_cast<TextureUsageType>(lhs) | static_cast<TextureUsageType>(rhs));
}

constexpr TextureUsage operator&(TextureUsage lhs, TextureUsage rhs) {
    return static_cast<TextureUsage>(static_cast<TextureUsageType>(lhs) & static_cast<TextureUsageType>(rhs));
}

enum class TextureFormat
{
    R8G8B8A8 = DXGI_FORMAT_R8G8B8A8_UNORM,
    R32G32B32A32 = DXGI_FORMAT_R32G32B32A32_FLOAT,
    D32 = DXGI_FORMAT_D32_FLOAT,
};

class Texture2D : public ResourceBase
{
public:
    Texture2D(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage = TextureUsage::All, HeapAllocationInfo* heapAllocInfo = nullptr);
    ~Texture2D();
    
    inline TextureFormat GetFormat() const { return mFormat; }
    inline uint32_t GetWidth() const { return mWidth; }
    inline uint32_t GetHeight() const { return mHeight; }
    inline uint32_t GetTextureSize() const { return mSize; }
    inline bool HasTextureUsage(TextureUsage usage) const { return static_cast<TextureUsageType>(mUsage & usage) == static_cast<TextureUsageType>(usage); }
    inline D3D12_RESOURCE_STATES GetCurrentResourceState() const { return GetResourceState(mCurrentUsage, mUseByPixelShader); }
    inline uint32_t GetRowSize() const { return GetWidth() * GetSizeForFormat(mFormat); }
    inline uint32_t GetRowOffset() const { return AlignPow2(GetRowSize(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) - GetRowSize(); }

    uint8_t* Map();

    void Unmap(CommandList& cmdList);
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
    void SetCurrentUsage(TextureUsage usage, bool pixelShader, std::vector<D3D12_RESOURCE_BARRIER>& barriers);
    std::optional<D3D12_CLEAR_VALUE> GetClearValue() const;

    static uint32_t GetSizeForFormat(TextureFormat format);

private:
    D3D12_RESOURCE_STATES GetResourceState(TextureUsage usage, bool pixelShader) const;
    void CreateViews();

    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    uint32_t mMipCount = 0;
    uint32_t mSize = 0;
    TextureFormat mFormat;
    TextureUsage mUsage;
    TextureUsage mCurrentUsage;
    bool mUseByPixelShader = false;

    std::unique_ptr<CPUDescriptorHandle> mRTVHandle;
    std::unique_ptr<CPUDescriptorHandle> mSRVHandle;
    std::unique_ptr<CPUDescriptorHandle> mDSVHandle;

    UploadBufferTemporaryRangeHandle mTemporaryMapResource = nullptr;

};

template<>
struct ResourceTraits<Texture2D>
{
    static const ResourceType Type = ResourceType::Texture2D;
};
