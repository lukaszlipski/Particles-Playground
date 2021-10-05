#pragma once
#include "fence.h"
#include "pix3.h"

class CPUDescriptorHeap;
class GPUDescriptorHeap;

enum class QueueType
{
    Direct = 0,
    Compute,
    Copy
};

class Graphic
{
public:
    ~Graphic();

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

    void PostStartup();
    void PreUpdate();
    void PostUpdate();
    ID3D12CommandQueue* GetQueue(QueueType type) const;
    ID3D12CommandAllocator* GetCommandAllocator(QueueType type, uint32_t index) const;
    ID3D12CommandAllocator* GetCurrentCommandAllocator(QueueType type) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetHandle();
    uint32_t GetHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

    CPUDescriptorHeap* GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
    GPUDescriptorHeap* GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);

    inline ID3D12Device* GetDevice() const { return mDevice; }
    inline ID3D12CommandQueue* GetDirectQueue() const { return mDirectQueue; }
    inline ID3D12CommandQueue* GetComputeQueue() const { return mComputeQueue; }
    inline ID3D12CommandQueue* GetCopyQueue() const { return mCopyQueue; }
    inline uint32_t GetCurrentFrameIndex() const { return mCurrentFrameIdx; }
    inline uint64_t GetCurrentFrameNumber() const { return mCurrentFrameNumber; }
    inline uint32_t GetRTVHandleSize() const { return mRTVHandleSize; }
    inline uint32_t GetCBVHandleSize() const { return mCBVHandleSize; }
    inline ID3D12Resource* GetRenderTarget(uint32_t index) const { return mRenderTargets[index]; }
    inline ID3D12Resource* GetCurrentRenderTarget() const { return mRenderTargets[GetCurrentFrameIndex()]; }
    inline ID3D12DescriptorHeap* GetSwapChainDescriptorHeap() const { return mSwapChainDescHeap; }
    inline Fence* GetFence(uint32_t index) { return mFences[index].get(); }
    inline Fence* GetCurrentFence() { return mFences[GetCurrentFrameIndex()].get(); }
    inline ID3D12CommandSignature* GetDefaultDrawCommandSignature() const { return mDefaultDrawCommandSignature; }
    inline ID3D12CommandSignature* GetDefaultDispatchCommandSignature() const { return mDefaultDispatchCommandSignature; }

    static D3D12_COMMAND_LIST_TYPE GetCommandListType(QueueType type);
    static constexpr uint32_t GetFrameCount() { return mFrameCount; }

private:
    explicit Graphic();

    bool EnableDebugLayer();
    bool FindBestAdapter();
    bool CreateSwapChain();
    bool CreateDefaultCommandSignatures();

    static const uint32_t mFrameCount = 3;
    uint32_t mRTVHandleSize = 0;
    uint32_t mCBVHandleSize = 0;
    uint32_t mCurrentFrameIdx = 0;
    uint64_t mCurrentFrameNumber = 0;

    ID3D12Debug* mDebugLayer = nullptr;
    IDXGIFactory4* mDXGIFactory = nullptr;
    IDXGIAdapter1* mAdapter = nullptr;
    ID3D12Device* mDevice = nullptr;
    IDXGISwapChain3* mSwapChain = nullptr;
    ID3D12DescriptorHeap* mSwapChainDescHeap = nullptr;
    ID3D12CommandQueue* mDirectQueue = nullptr;
    ID3D12CommandQueue* mComputeQueue = nullptr;
    ID3D12CommandQueue* mCopyQueue = nullptr;
    ID3D12CommandSignature* mDefaultDrawCommandSignature = nullptr;
    ID3D12CommandSignature* mDefaultDispatchCommandSignature = nullptr;

    std::unique_ptr<CPUDescriptorHeap> mCPUDescriptorHeapCBV;
    std::unique_ptr<GPUDescriptorHeap> mGPUDescriptorHeapCBV;

    std::unique_ptr<CPUDescriptorHeap> mCPUDescriptorHeapRTV;
    std::unique_ptr<CPUDescriptorHeap> mCPUDescriptorHeapDSV;

    std::array<ID3D12CommandAllocator*, mFrameCount> mDirectCommandAllocator;
    std::array<ID3D12CommandAllocator*, mFrameCount> mComputeCommandAllocator;
    std::array<ID3D12CommandAllocator*, mFrameCount> mCopyCommandAllocator;
    std::array<ID3D12Resource*, mFrameCount> mRenderTargets;
    std::array<upFence, mFrameCount> mFences;

};
