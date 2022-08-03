#pragma once
#include "System/window.h"
#include "System/graphic.h"
#include "System/psomanager.h"
#include "System/meshmanager.h"
#include "System/commandlist.h"
#include "System/gpubuffer.h"
#include "System/texture.h"
#include "System/sampler.h"
#include "System/cpudescriptorheap.h"
#include "System/gpudescriptorheap.h"
#include "System/globaltimer.h"
#include "System/shaderparameters.h"
#include "System/pipelinestate.h"
#include "System/shaderparameterslayout.h"
#include "System/shadermanager.h"
#include "System/transientresourceallocator.h"
#include "System/rendergraph.h"
#include "Graphics/camera.h"
#include "Graphics/gpuparticlesystem.h"
#include "Utilities/memory.h"
#include "Utilities/debug.h"
#include "Graphics/RenderGraph/gpuparticlesystemrendernodes.h"
#include "Graphics/RenderGraph/fullscreennodes.h"
#include "Graphics/RenderGraph/miscnodes.h"
#include "Shaders/bindlesscommon.hlsli"

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

    inline GlobalTimer& GetTimer() { return mTimer; }

private:
    explicit Engine() = default;

    GlobalTimer mTimer;

};
