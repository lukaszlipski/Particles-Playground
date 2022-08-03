#pragma once
#include "Utilities/freelistallocator.h"
#include "Graphics/gpuemitter.h"
#include "Graphics/gpuemittertemplate.h"

class GPUBuffer;
class CommandList;
class Texture2D;

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

    void PostUpdate();

    [[nodiscard]] std::vector<GPUEmitter*> GetEnabledEmitters() const;
    [[nodiscard]] std::vector<GPUEmitter*> GetDirtyEmitters() const;
    [[nodiscard]] std::vector<GPUEmitter*> GetEmitters() const;

    inline GPUEmitterHandle CreateEmitter(GPUEmitterTemplateHandle emitterTemplate, uint32_t maxParticles) { return mEmittersPool.AllocateObject(this, emitterTemplate, maxParticles); }
    inline void FreeEmitter(GPUEmitterHandle& handle) { mEmittersPool.FreeObject(handle); }
    inline GPUEmitter* GetEmitter(GPUEmitterHandle handle) { return mEmittersPool.GetObject(handle); }

    [[nodiscard]] inline GPUEmitterTemplateHandle CreateEmitterTemplate() { return mEmitterTemplatesPool.AllocateObject(); }
    inline void FreeEmitterTemplate(GPUEmitterTemplateHandle& handle) { mEmitterTemplatesPool.FreeObject(handle); }
    inline GPUEmitterTemplate* GetEmitterTemplate(GPUEmitterTemplateHandle handle) { return mEmitterTemplatesPool.GetObject(handle); }

    inline Range AllocateParticles(uint32_t maxParticles) { return mParticlesAllocator.Allocate(maxParticles); }
    inline void FreeParticles(Range& allocation) { return mParticlesAllocator.Free(allocation); }

    [[nodiscard]] inline uint32_t GetRandomNumber() { return mRNG.GetRandom(); }

    inline GPUBuffer* GetParticlesDataBuffer() const { return mParticlesDataBuffer.get(); }
    inline GPUBuffer* GetFreeIndicesBuffer() const { return mFreeIndicesBuffer.get(); }
    inline GPUBuffer* GetEmitterIndexBuffer() const { return mEmitterIndexBuffer.get(); }
    inline GPUBuffer* GetEmitterConstantBuffer() const { return mEmitterConstantBuffer.get(); }
    inline GPUBuffer* GetEmitterStatusBuffer() const { return mEmitterStatusBuffer.get(); }
    inline GPUBuffer* GetDrawIndirectBuffer() const { return mDrawIndirectBuffer.get(); }

private:
    void UpdateDirtyEmitters(CommandList& commandList);
    void UpdateEmitters(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters);
    void SpawnParticles(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters);
    void UpdateParticles(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters);

    ObjectPool<GPUEmitterTemplate> mEmitterTemplatesPool;
    ObjectPool<GPUEmitter> mEmittersPool;

    FreeListAllocator<FirstFitStrategy> mParticlesAllocator;
    std::unique_ptr<GPUBuffer> mParticlesDataBuffer;
    std::unique_ptr<GPUBuffer> mFreeIndicesBuffer;

    std::unique_ptr<GPUBuffer> mEmitterIndexBuffer;
    std::unique_ptr<GPUBuffer> mEmitterConstantBuffer;
    std::unique_ptr<GPUBuffer> mEmitterStatusBuffer;

    std::unique_ptr<GPUBuffer> mDrawIndirectBuffer;

    // Note: Use different RNG than EmitterUpdate shader
    RandomNumberGenerator<RngType::xxHash32> mRNG;

};
