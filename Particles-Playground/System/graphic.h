#pragma once

class Graphic
{
public:
    Graphic(const Graphic&) = delete;
    Graphic(Graphic&&) = delete;

    Graphic& operator=(const Graphic&) = delete;
    Graphic& operator=(Graphic&&) = delete;

    bool Startup();
    bool Shutdown();

    static Graphic& Get()
    {
        static Graphic* instance = new Graphic();
        return *instance;
    }

    inline ID3D12Device* GetDevice() const { return mDevice; }
    inline ID3D12CommandQueue* GetDirectQueue() const { return mDirectQueue; }
    inline ID3D12CommandQueue* GetComputeQueue() const { return mComputeQueue; }
    inline ID3D12CommandQueue* GetCopyQueue() const { return mCopyQueue; }
    inline uint32_t GetFrameCount() const { return mFrameCount; }
    inline uint32_t GetCurrentFrameIndex() const { return mSwapChain->GetCurrentBackBufferIndex(); }
    inline uint32_t GetRTVHandleSize() const { return mRTVHandleSize; }
    inline uint32_t GetCBVHandleSize() const { return mCBVHandleSize; }
    inline ID3D12Resource* GetRenderTarget(uint32_t index) const { return mRenderTargets[index]; }
    inline ID3D12DescriptorHeap* GetSwapChainDescriptorHeap() const { return mSwapChainDescHeap; }

private:
    explicit Graphic() = default;

    bool EnableDebugLayer();
    bool FindBestAdapter();
    bool CreateSwapChain();

    static const uint32_t mFrameCount = 3;
    uint32_t mRTVHandleSize = 0;
    uint32_t mCBVHandleSize = 0;

    ID3D12Debug* mDebugLayer = nullptr;
    IDXGIFactory4* mDXGIFactory = nullptr;
    IDXGIAdapter1* mAdapter = nullptr;
    ID3D12Device* mDevice = nullptr;
    IDXGISwapChain3* mSwapChain = nullptr;
    ID3D12DescriptorHeap* mSwapChainDescHeap = nullptr;
    ID3D12CommandQueue* mDirectQueue = nullptr;
    ID3D12CommandQueue* mComputeQueue = nullptr;
    ID3D12CommandQueue* mCopyQueue = nullptr;

    std::array<ID3D12Resource*, mFrameCount> mRenderTargets;

};
