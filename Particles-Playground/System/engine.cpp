#include "engine.h"
#include "gpubufferuploadmanager.h"

void Engine::Startup()
{
    Window::Get().Startup();
    Graphic::Get().Startup();
    PSOManager::Get().Startup();
    GPUBufferUploadManager::Get().Startup();
    MeshManager::Get().Startup();
}

void Engine::PostStartup()
{
    Graphic::Get().PostStartup();
    Window::Get().Show();
}

void Engine::PreUpdate()
{
    Window::Get().PreUpdate();
    Graphic::Get().PreUpdate();
    GPUBufferUploadManager::Get().PreUpdate();
}

void Engine::PostUpdate()
{
    Graphic::Get().PostUpdate();
}

void Engine::PreShutdown()
{
    Graphic::Get().GetCurrentFence()->Flush(QueueType::Direct);
}

void Engine::Shutdown()
{
    MeshManager::Get().Shutdown();
    GPUBufferUploadManager::Get().Shutdown();
    PSOManager::Get().Shutdown();
    Graphic::Get().Shutdown();
    Window::Get().Shutdown();
}
