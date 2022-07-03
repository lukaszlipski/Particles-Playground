#include "texture.h"
#include "graphic.h"
#include "cpudescriptorheap.h"
#include "commandlist.h"

Texture2D::Texture2D(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, HeapAllocationInfo* heapAllocInfo)
    : ResourceBase(ResourceTraits<Texture2D>::Type)
    , mWidth(width)
    , mHeight(height)
    , mFormat(format)
    , mUsage(usage)
{
    ID3D12Device* const device = Graphic::Get().GetDevice();

    if      (HasTextureUsage(TextureUsage::ShaderResource)) { mCurrentUsage = TextureUsage::ShaderResource; }
    else if (HasTextureUsage(TextureUsage::RenderTarget))   { mCurrentUsage = TextureUsage::RenderTarget; }
    else if (HasTextureUsage(TextureUsage::CopyDst))        { mCurrentUsage = TextureUsage::CopyDst; }
    else if (HasTextureUsage(TextureUsage::CopySrc))        { mCurrentUsage = TextureUsage::CopySrc; }
    else if (HasTextureUsage(TextureUsage::DepthWrite))     { mCurrentUsage = TextureUsage::DepthWrite; }
    else if (HasTextureUsage(TextureUsage::DepthRead))      { mCurrentUsage = TextureUsage::DepthRead; }
    else if (HasTextureUsage(TextureUsage::All))            { mCurrentUsage = TextureUsage::All; }

    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    std::optional<D3D12_CLEAR_VALUE> clearValue = GetClearValue();

    if (HasTextureUsage(TextureUsage::RenderTarget))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }

    if (HasTextureUsage(TextureUsage::DepthWrite) || HasTextureUsage(TextureUsage::DepthRead))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }

    const uint32_t mipCount = 0;

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(static_cast<DXGI_FORMAT>(format), width, height, 1, mipCount, 1, 0, flags);

    HRESULT hr = E_HANDLE;
    if (heapAllocInfo)
    {
        Assert(heapAllocInfo->IsValid());
        hr = device->CreatePlacedResource(heapAllocInfo->mHeap, heapAllocInfo->mOffset, &desc, GetCurrentResourceState(),
            clearValue.has_value() ? &clearValue.value() : nullptr, IID_PPV_ARGS(&mResource));
    }
    else
    {
        CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_DEFAULT);
        hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, GetCurrentResourceState(), 
            clearValue.has_value() ? &clearValue.value() : nullptr, IID_PPV_ARGS(&mResource));
    }
    Assert(SUCCEEDED(hr));

    mSize = static_cast<uint32_t>(GetRequiredIntermediateSize(mResource, 0, mipCount + 1));

    CreateViews();
}

Texture2D::~Texture2D()
{
    if (mRTVHandle) { Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->Free(*mRTVHandle); }
    if (mSRVHandle) { Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(*mSRVHandle); }
    if (mDSVHandle) { Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->Free(*mDSVHandle); }
}

uint8_t* Texture2D::Map()
{
    Assert(!mTemporaryMapResource); // Tried to map twice

    const uint32_t size = mSize;

    mTemporaryMapResource = GPUBufferUploadManager::Get().Reserve(size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    return mTemporaryMapResource->Map();
}

void Texture2D::Unmap(CommandList& cmdList)
{
    Assert(mTemporaryMapResource); // Tried to unmap without mapping

    const uint64_t startRange = mTemporaryMapResource->GetStartRange();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Offset = AlignPow2(startRange, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    footprint.Footprint = CD3DX12_SUBRESOURCE_FOOTPRINT(static_cast<DXGI_FORMAT>(GetFormat()), GetWidth(), GetHeight(), 1, AlignPow2(GetRowSize(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));

    mTemporaryMapResource->Unmap();

    CD3DX12_RESOURCE_BARRIER before = CD3DX12_RESOURCE_BARRIER::Transition(GetResource(), GetCurrentResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
    CD3DX12_RESOURCE_BARRIER after = CD3DX12_RESOURCE_BARRIER::Transition(GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, GetCurrentResourceState());

    CD3DX12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(GetResource(), 0);
    CD3DX12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(GPUBufferUploadManager::Get().GetResource(), footprint);
    
    cmdList->ResourceBarrier(1, &before);
    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    cmdList->ResourceBarrier(1, &after);

    mTemporaryMapResource = nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture2D::GetRTV() const
{
    Assert(HasTextureUsage(TextureUsage::RenderTarget));
    return *mRTVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture2D::GetSRV() const
{
    Assert(HasTextureUsage(TextureUsage::ShaderResource));
    return *mSRVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture2D::GetDSV() const
{
    Assert(HasTextureUsage(TextureUsage::DepthRead) || HasTextureUsage(TextureUsage::DepthWrite));
    return *mDSVHandle;
}

void Texture2D::SetCurrentUsage(TextureUsage usage, bool pixelShader, std::vector<D3D12_RESOURCE_BARRIER>& barriers)
{
    Assert(IsPow2(static_cast<TextureUsageType>(usage))); // Only one bit can be set
    Assert(HasTextureUsage(usage));

    const D3D12_RESOURCE_STATES stateBefore = GetCurrentResourceState();
    const D3D12_RESOURCE_STATES stateAfter = GetResourceState(usage, pixelShader);
    
    if (stateBefore != stateAfter)
    {
        D3D12_RESOURCE_BARRIER& barrier = barriers.emplace_back();
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetResource(), stateBefore, stateAfter);
    }
    mCurrentUsage = usage;
    mUseByPixelShader = pixelShader;
}

std::optional<D3D12_CLEAR_VALUE> Texture2D::GetClearValue() const
{
    std::optional<D3D12_CLEAR_VALUE> clearValue;

    if (HasTextureUsage(TextureUsage::RenderTarget))
    {
        const FLOAT color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValue = CD3DX12_CLEAR_VALUE(static_cast<DXGI_FORMAT>(mFormat), color);
    }
    if (HasTextureUsage(TextureUsage::DepthWrite) || HasTextureUsage(TextureUsage::DepthRead))
    {
        clearValue = CD3DX12_CLEAR_VALUE(static_cast<DXGI_FORMAT>(mFormat), 1, 0);
    }

    return clearValue;
}

uint32_t Texture2D::GetSizeForFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8G8B8A8:
        return sizeof(uint32_t);
    case TextureFormat::R32G32B32A32:
        return sizeof(float) * 4;
    case TextureFormat::D32:
        return sizeof(float);
    default:
        Assert(0);
    }

    return 0;
}

D3D12_RESOURCE_STATES Texture2D::GetResourceState(TextureUsage usage, bool pixelShader) const
{
    Assert(IsPow2(static_cast<TextureUsageType>(usage))); // Only one bit can be set

    switch (usage)
    {
    case TextureUsage::ShaderResource:
        return pixelShader ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case TextureUsage::RenderTarget:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case TextureUsage::CopyDst:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case TextureUsage::CopySrc:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case TextureUsage::DepthRead:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case TextureUsage::DepthWrite:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case TextureUsage::All:
        return D3D12_RESOURCE_STATE_GENERIC_READ;
    default:
        Assert(0);
    }
    return D3D12_RESOURCE_STATE_GENERIC_READ;
}

void Texture2D::CreateViews()
{
    if (HasTextureUsage(TextureUsage::RenderTarget))
    {
        mRTVHandle = std::make_unique<CPUDescriptorHandle>(Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->Allocate());

        D3D12_RENDER_TARGET_VIEW_DESC desc{};
        desc.Format = static_cast<DXGI_FORMAT>(mFormat);
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;
        desc.Texture2D.PlaneSlice = 0;

        Graphic::Get().GetDevice()->CreateRenderTargetView(mResource, &desc, *mRTVHandle);
    }
    if (HasTextureUsage(TextureUsage::ShaderResource))
    {
        mSRVHandle = std::make_unique<CPUDescriptorHandle>(Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate());

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = mMipCount + 1;

        Graphic::Get().GetDevice()->CreateShaderResourceView(mResource, &desc, *mSRVHandle);
    }
    if (HasTextureUsage(TextureUsage::DepthWrite) || HasTextureUsage(TextureUsage::DepthRead))
    {
        mDSVHandle = std::make_unique<CPUDescriptorHandle>(Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->Allocate());

        D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
        desc.Format = static_cast<DXGI_FORMAT>(mFormat);
        desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        desc.Flags = D3D12_DSV_FLAG_NONE;
        desc.Texture2D.MipSlice = 0;
        
        Graphic::Get().GetDevice()->CreateDepthStencilView(mResource, &desc, *mDSVHandle);
    }
}
