#include "graphic.h"

bool Graphic::Startup()
{
    const bool debugLayerEnabled = EnableDebugLayer();

    const UINT factoryFlags = debugLayerEnabled ? DXGI_CREATE_FACTORY_DEBUG : 0;
    if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mDXGIFactory)))) { return false; }

    if (!FindBestAdapter()) { return false; }

    if (FAILED(D3D12CreateDevice(mAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)))) { return false; }

    return true;
}

bool Graphic::Shutdown()
{
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
