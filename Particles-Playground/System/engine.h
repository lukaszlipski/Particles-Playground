#pragma once
#include "window.h"
#include "graphic.h"
#include "psomanager.h"
#include "meshmanager.h"
#include "commandlist.h"

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
