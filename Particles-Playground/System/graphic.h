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

private:
    explicit Graphic() = default;

    bool EnableDebugLayer();
    bool FindBestAdapter();

    ID3D12Debug* mDebugLayer = nullptr;
    IDXGIFactory4* mDXGIFactory = nullptr;
    IDXGIAdapter1* mAdapter = nullptr;
    ID3D12Device* mDevice = nullptr;

};
