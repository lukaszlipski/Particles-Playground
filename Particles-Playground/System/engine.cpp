#include "engine.h"

void Engine::Startup()
{
    Window::Get().Startup();
    Graphic::Get().Startup();
    PSOManager::Get().Startup();
    MeshManager::Get().Startup();
}

void Engine::PostStartup()
{
    MeshManager::Get().PostStartup();
    Window::Get().Show();
}

void Engine::PreUpdate()
{
    Window::Get().PreUpdate();
    Graphic::Get().PreUpdate();
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
    PSOManager::Get().Shutdown();
    Graphic::Get().Shutdown();
    Window::Get().Shutdown();
}
