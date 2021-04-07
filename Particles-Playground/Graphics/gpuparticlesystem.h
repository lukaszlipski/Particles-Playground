#pragma once
#include "Utilities/freelistallocator.h"
#include "gpuemitter.h"

class GPUBuffer;
class CommandList;
class Texture2D;

struct EmitterUpdateConstants
{
    uint32_t emittersCount;
    float deltaTime;
};

class GPUParticleSystem
{
public:
    static const uint32_t MaxParticles = 4096;
    static const uint32_t MaxEmitters = 64;

    GPUParticleSystem();
    ~GPUParticleSystem() = default;
    GPUParticleSystem(const GPUParticleSystem&) = delete;
    GPUParticleSystem(GPUParticleSystem&&) = default;

    GPUParticleSystem& operator=(const GPUParticleSystem&) = delete;
    GPUParticleSystem& operator=(GPUParticleSystem&&) = default;

    void Init();
    void Free();

    void Update(CommandList& commandList);
    void UpdateDirtyEmitters(CommandList& commandList);
    void UpdateEmitters(CommandList& commandList, const std::vector<GPUEmitterHandle>& enabledEmitters);
    void SpawnParticles(CommandList& commandList, const std::vector<GPUEmitterHandle>& enabledEmitters);
    void UpdateParticles(CommandList& commandList, const std::vector<GPUEmitterHandle>& enabledEmitters);
    void DrawParticles(CommandList& commandList, GPUBuffer* cameraBuffer, Texture2D* renderTarget);

    GPUEmitterHandle CreateEmitter(uint32_t maxParticles);
    void FreeEmitter(GPUEmitterHandle& handle);

    inline Range AllocateEmitter() { return mEmittersAllocator.Allocate(1); }
    inline void FreeEmitter(Range& allocation) { mEmittersAllocator.Free(allocation); }

    inline Range AllocateParticles(uint32_t maxParticles) { return mParticlesAllocator.Allocate(maxParticles); }
    inline void FreeParticles(Range& allocation) { return mParticlesAllocator.Free(allocation); }

    GPUBuffer* GetDrawIndirectBuffer() const { return mDrawIndirectBuffer.get(); }
    GPUBuffer* GetParticlesDataBuffer() const { return mParticlesDataBuffer.get(); }
    GPUBuffer* GetIndicesBuffer() const { return mIndicesBuffer.get(); }

private:
    std::vector<std::unique_ptr<GPUEmitter>> mEmitters;

    FreeListAllocator<FirstFitStrategy> mParticlesAllocator;
    std::unique_ptr<GPUBuffer> mParticlesDataBuffer;
    std::unique_ptr<GPUBuffer> mIndicesBuffer;
    std::unique_ptr<GPUBuffer> mFreeIndicesBuffer;

    FreeListAllocator<FirstFitStrategy> mEmittersAllocator;
    std::unique_ptr<GPUBuffer> mEmitterIndexBuffer;
    std::unique_ptr<GPUBuffer> mEmitterConstantBuffer;
    std::unique_ptr<GPUBuffer> mEmitterStatusBuffer;

    std::unique_ptr<GPUBuffer> mDrawIndirectBuffer;
    std::unique_ptr<GPUBuffer> mSpawnIndirectBuffer;

};
