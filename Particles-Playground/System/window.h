#pragma once

class Window
{
public:
    Window(const Window&) = delete;
    Window(Window&&) = delete;

    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

    bool Startup();
    bool Shutdown();

    void PreUpdate();

    static Window& Get()
    {
        static Window* instance = new Window();
        return *instance;
    }

    void Close();
    void Show();

    inline bool IsRunning() const { return mIsRunning; }
    inline HWND GetHandle() const { return mHandle; }
    inline int32_t GetWidth() const { return mWidth; }
    inline int32_t GetHeight() const { return mHeight; }

private:
    explicit Window() = default;

    int32_t mWidth = 1280;
    int32_t mHeight = 720;
    HWND mHandle = nullptr;
    bool mIsRunning = false;

    friend LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};
