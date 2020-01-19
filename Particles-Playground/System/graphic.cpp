#include "graphic.h"
#include "window.h"

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

    // #TODO: Implement full screen support
    const HWND handle = Window::Get().GetHandle();
    mDXGIFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER);

    return true;
}

bool Graphic::Shutdown()
{
    if (mCopyQueue) { mCopyQueue->Release(); }
    if (mComputeQueue) { mComputeQueue->Release(); }
    if (mDirectQueue) { mDirectQueue->Release(); }

    for (ID3D12Resource* renderTarget : mRenderTargets)
    {
        if (renderTarget) { renderTarget->Release(); }
    }

    if (mSwapChainDescHeap) { mSwapChainDescHeap->Release(); }
    if (mSwapChain) { mSwapChain->Release(); }
    if (mDevice) { mDevice->Release(); }
    if (mAdapter) { mAdapter->Release(); }
    if (mDXGIFactory) { mDXGIFactory->Release(); }
    if (mDebugLayer) { mDebugLayer->Release(); }

    return true;
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

    return true;
}
