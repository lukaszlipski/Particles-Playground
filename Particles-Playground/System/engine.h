#pragma once
#include "System/window.h"
#include "System/graphic.h"
#include "System/psomanager.h"
#include "System/meshmanager.h"
#include "System/commandlist.h"
#include "System/gpubuffer.h"
#include "System/cpudescriptorheap.h"
#include "System/gpudescriptorheap.h"
#include "Graphics/camera.h"
#include "Utilities/memory.h"

class Engine
{
public:
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;

    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    void Startup();
    void PostStartup();

    void PreUpdate();
    void PostUpdate();

    void PreShutdown();
    void Shutdown();

    static Engine& Get()
    {
        static Engine* instance = new Engine();
        return *instance;
    }

private:
    explicit Engine() = default;

};
