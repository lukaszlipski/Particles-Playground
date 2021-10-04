#pragma once
#include "Utilities/freelistallocator.h"
#include "Graphics/gpuemitter.h"
#include "Graphics/gpuemittertemplate.h"

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
    static const uint32_t MaxEmitterTemplates = 16;

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
    void UpdateEmitters(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters);
    void SpawnParticles(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters);
    void UpdateParticles(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters);
    void DrawParticles(CommandList& commandList, GPUBuffer* cameraBuffer, Texture2D* renderTarget);

    inline GPUEmitterHandle CreateEmitter(GPUEmitterTemplateHandle emitterTemplate, uint32_t maxParticles) { return mEmittersPool.AllocateObject(this, emitterTemplate, maxParticles); }
    inline void FreeEmitter(GPUEmitterHandle& handle) { mEmittersPool.FreeObject(handle); }
    inline GPUEmitter* GetEmitter(GPUEmitterHandle handle) { return mEmittersPool.GetObject(handle); }

    inline [[nodiscard]] GPUEmitterTemplateHandle CreateEmitterTemplate() { return mEmitterTemplatesPool.AllocateObject(); }
    inline void FreeEmitterTemplate(GPUEmitterTemplateHandle& handle) { mEmitterTemplatesPool.FreeObject(handle); }
    inline GPUEmitterTemplate* GetEmitterTemplate(GPUEmitterTemplateHandle handle) { return mEmitterTemplatesPool.GetObject(handle); }

    inline Range AllocateParticles(uint32_t maxParticles) { return mParticlesAllocator.Allocate(maxParticles); }
    inline void FreeParticles(Range& allocation) { return mParticlesAllocator.Free(allocation); }

    inline [[nodiscard]] uint32_t GetRandomNumber() { return mRNG.GetRandom(); }

    GPUBuffer* GetDrawIndirectBuffer() const { return mDrawIndirectBuffer.get(); }
    GPUBuffer* GetParticlesDataBuffer() const { return mParticlesDataBuffer.get(); }
    GPUBuffer* GetIndicesBuffer() const { return mIndicesBuffer.get(); }

private:
    ObjectPool<GPUEmitterTemplate> mEmitterTemplatesPool;
    ObjectPool<GPUEmitter> mEmittersPool;

    FreeListAllocator<FirstFitStrategy> mParticlesAllocator;
    std::unique_ptr<GPUBuffer> mParticlesDataBuffer;
    std::unique_ptr<GPUBuffer> mIndicesBuffer;
    std::unique_ptr<GPUBuffer> mFreeIndicesBuffer;

    std::unique_ptr<GPUBuffer> mEmitterIndexBuffer;
    std::unique_ptr<GPUBuffer> mEmitterConstantBuffer;
    std::unique_ptr<GPUBuffer> mEmitterStatusBuffer;

    std::unique_ptr<GPUBuffer> mDrawIndirectBuffer;
    std::unique_ptr<GPUBuffer> mSpawnIndirectBuffer;

    // Note: Use different RNG than EmitterUpdate shader
    RandomNumberGenerator<RngType::xxHash32> mRNG;

};
