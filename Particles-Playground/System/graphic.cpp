#include "graphic.h"
#include "window.h"
#include "cpudescriptorheap.h"
#include "gpudescriptorheap.h"

Graphic::~Graphic() = default;
Graphic::Graphic() = default;

bool Graphic::Startup()
{
    const bool debugLayerEnabled = EnableDebugLayer();

    const UINT factoryFlags = debugLayerEnabled ? DXGI_CREATE_FACTORY_DEBUG : 0;
    if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mDXGIFactory)))) { return false; }

    if (!FindBestAdapter()) { return false; }

    if (FAILED(D3D12CreateDevice(mAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)))) { return false; }

    // Get RTV and CB descriptor size for current device
    mRTVHandleSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mCBVHandleSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create queues, one for each type
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mDirectQueue)))) { return false; }

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    if (FAILED(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeQueue)))) { return false; }

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    if (FAILED(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyQueue)))) { return false; }

    if (!CreateSwapChain()) { return false; }

    for (ID3D12CommandAllocator*& allocator : mDirectCommandAllocator)
    {
        if (FAILED(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)))) { return false; }
    }
    for (ID3D12CommandAllocator*& allocator : mComputeCommandAllocator)
    {
        if (FAILED(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&allocator)))) { return false; }
    }
    for (ID3D12CommandAllocator*& allocator : mCopyCommandAllocator)
    {
        if (FAILED(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&allocator)))) { return false; }
    }

    for (upFence& fence : mFences)
    {
        fence = std::make_unique<Fence>();
    }

    // #TODO: Implement full screen support
    const HWND handle = Window::Get().GetHandle();
    mDXGIFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER);

    mCPUDescriptorHeapCBV = std::make_unique<CPUDescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mCPUDescriptorHeapRTV = std::make_unique<CPUDescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    mGPUDescriptorHeapCBV = std::make_unique<GPUDescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    if (!CreateDefaultCommandSignatures()) { return false; }

    return true;
}

bool Graphic::Shutdown()
{
    for (upFence& fence : mFences) { fence.reset(); }

    for (ID3D12CommandAllocator* allocator : mDirectCommandAllocator) { allocator->Release(); }
    for (ID3D12CommandAllocator* allocator : mComputeCommandAllocator) { allocator->Release(); }
    for (ID3D12CommandAllocator* allocator : mCopyCommandAllocator) { allocator->Release(); }

    if (mDefaultDrawCommandSignature) { mDefaultDrawCommandSignature->Release(); }
    if (mDefaultDispatchCommandSignature) { mDefaultDispatchCommandSignature->Release(); }

    if (mCopyQueue) { mCopyQueue->Release(); }
    if (mComputeQueue) { mComputeQueue->Release(); }
    if (mDirectQueue) { mDirectQueue->Release(); }

    for (ID3D12Resource* renderTarget : mRenderTargets)
    {
        if (renderTarget) { renderTarget->Release(); }
    }

    if (mCPUDescriptorHeapCBV) { mCPUDescriptorHeapCBV.reset(); }
    if (mCPUDescriptorHeapRTV) { mCPUDescriptorHeapRTV.reset(); }

    if (mGPUDescriptorHeapCBV) { mGPUDescriptorHeapCBV.reset(); }

    if (mSwapChainDescHeap) { mSwapChainDescHeap->Release(); }
    if (mSwapChain) { mSwapChain->Release(); }
    if (mDevice) { mDevice->Release(); }
    if (mAdapter) { mAdapter->Release(); }
    if (mDXGIFactory) { mDXGIFactory->Release(); }
    if (mDebugLayer) { mDebugLayer->Release(); }

    return true;
}

void Graphic::PostStartup()
{
    Graphic::Get().GetCurrentFence()->Signal(QueueType::Direct);
}

void Graphic::PreUpdate()
{
    Graphic::Get().GetCurrentFence()->WaitOnCPU();

    ID3D12CommandAllocator* allocator = Graphic::Get().GetCurrentCommandAllocator(QueueType::Copy);
    allocator->Reset();

    allocator = Graphic::Get().GetCurrentCommandAllocator(QueueType::Compute);
    allocator->Reset();

    allocator = Graphic::Get().GetCurrentCommandAllocator(QueueType::Direct);
    allocator->Reset();

    mGPUDescriptorHeapCBV->ReleaseUnusedDescriptorHandles();
}

void Graphic::PostUpdate()
{
    Graphic::Get().GetCurrentFence()->Signal(QueueType::Direct);
    mSwapChain->Present(1, 0);
    mCurrentFrameIdx = mSwapChain->GetCurrentBackBufferIndex();
    ++mCurrentFrameNumber;
}

ID3D12CommandQueue* Graphic::GetQueue(QueueType type) const
{
    switch (type)
    {
    case QueueType::Direct:
        return mDirectQueue;
    case QueueType::Compute:
        return mComputeQueue;
    case QueueType::Copy:
        return mCopyQueue;
    default:
        return nullptr;
    }
}

ID3D12CommandAllocator* Graphic::GetCommandAllocator(QueueType type, uint32_t index) const
{
    switch (type)
    {
    case QueueType::Direct:
        return mDirectCommandAllocator[index];
    case QueueType::Compute:
        return mComputeCommandAllocator[index];
    case QueueType::Copy:
        return mCopyCommandAllocator[index];
    default:
        return nullptr;
    }
}

ID3D12CommandAllocator* Graphic::GetCurrentCommandAllocator(QueueType type) const
{
    return GetCommandAllocator(type, GetCurrentFrameIndex());
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Graphic::GetCurrentRenderTargetHandle()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(mSwapChainDescHeap->GetCPUDescriptorHandleForHeapStart(), GetCurrentFrameIndex(), GetRTVHandleSize());
}

uint32_t Graphic::GetHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return Graphic::Get().GetCBVHandleSize();
    default:
        return Graphic::Get().GetRTVHandleSize();
    }
}

CPUDescriptorHeap* Graphic::GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return mCPUDescriptorHeapCBV.get();
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        return mCPUDescriptorHeapRTV.get();
    default:
        assert(false);
    }
    return nullptr;
}

GPUDescriptorHeap* Graphic::GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return mGPUDescriptorHeapCBV.get();
    default:
        assert(false);
    }
    return nullptr;
}

D3D12_COMMAND_LIST_TYPE Graphic::GetCommandListType(QueueType type)
{
    switch (type)
    {
    case QueueType::Direct:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case QueueType::Compute:
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case QueueType::Copy:
        return D3D12_COMMAND_LIST_TYPE_COPY;
    default:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
}

bool Graphic::EnableDebugLayer()
{
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugLayer))))
    {
        mDebugLayer->EnableDebugLayer();
        return true;
    }

    return false;
}

bool Graphic::FindBestAdapter()
{
    SIZE_T currentHighestMem = 0;
    IDXGIAdapter1* iter = nullptr;
    for (int32_t i = 0; mDXGIFactory->EnumAdapters1(i, &iter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        iter->GetDesc1(&desc);

        const bool softwareAdapter = desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
        const bool supportsDx12 = SUCCEEDED(D3D12CreateDevice(iter, D3D_FEATURE_LEVEL_11_0, IID_ID3D12Device, nullptr));
        const bool hasMoreDedicatedMem = currentHighestMem < desc.DedicatedVideoMemory;

        if (supportsDx12 && !softwareAdapter && hasMoreDedicatedMem)
        {
            if (mAdapter) { mAdapter->Release(); }

            currentHighestMem = desc.DedicatedVideoMemory;
            mAdapter = iter;
        }
        else
        {
            iter->Release();
        }
    }

    return mAdapter ? true : false;
}

bool Graphic::CreateSwapChain()
{
    IDXGISwapChain1* swapChain = nullptr;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount = mFrameCount;
    swapChainDesc.Width = 0;
    swapChainDesc.Height = 0;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    const HWND handle = Window::Get().GetHandle();
    if (FAILED(mDXGIFactory->CreateSwapChainForHwnd(mDirectQueue, handle, &swapChainDesc, nullptr, nullptr, &swapChain))) { return false; }

    swapChain->QueryInterface(IID_PPV_ARGS(&mSwapChain));
    swapChain->Release();

    // Create a descriptor heap for swap chain's RTVs
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = mFrameCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    if(FAILED(mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mSwapChainDescHeap)))) { return false; }

    // Create render target views for swap chain images
    CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle{ mSwapChainDescHeap->GetCPUDescriptorHandleForHeapStart() };
    for (int32_t i = 0; i < mFrameCount; ++i)
    {
        ID3D12Resource*& renderTarget = mRenderTargets[i];

        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget));
        mDevice->CreateRenderTargetView(renderTarget, nullptr, descHandle);

        descHandle.Offset(mRTVHandleSize);
    }

    mCurrentFrameIdx = mSwapChain->GetCurrentBackBufferIndex();

    return true;
}

bool Graphic::CreateDefaultCommandSignatures()
{
    std::array<D3D12_INDIRECT_ARGUMENT_DESC, 1> indirectArgs;
    indirectArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC desc{};
    desc.pArgumentDescs = indirectArgs.data();
    desc.NumArgumentDescs = static_cast<uint32_t>(indirectArgs.size());
    desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);

    HRESULT drawRes = mDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&mDefaultDrawCommandSignature));

    indirectArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    
    desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);

    HRESULT dispatchRes = mDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&mDefaultDispatchCommandSignature));

    return SUCCEEDED(drawRes) && SUCCEEDED(dispatchRes);
}
