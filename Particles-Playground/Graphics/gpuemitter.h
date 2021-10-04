#pragma once
#include "Graphics/gpuemittertemplate.h"
#include "Utilities/objectpool.h"

// #TODO: implement SOA
struct ParticleData
{
    XMFLOAT3 Position;
    float LifeTime;
    XMFLOAT3 Velocity;
    float Scale;
    XMFLOAT4 Color;
};

struct EmitterConstantData
{
    uint32_t MaxParticles = 0;
    float SpawnRate = 0;
    float LifeTime = 0;
    uint32_t IndicesOffset = 0;
    XMFLOAT4 Color = { 1, 1, 1, 1 };
    XMFLOAT3 Position = { 0, 0, 0 };
};

struct EmitterStatusData
{
    uint32_t CurrentSeed = 0;
    uint32_t AliveParticles = 0;
    uint32_t ParticlesToSpawn = 0;
    uint32_t ParticlesToUpdate = 0;
    float SpawnAccTime = 0;
};

class GPUParticleSystem;
class CommandList;

class GPUEmitter : public IObject<GPUEmitter>
{
public:
    GPUEmitter(GPUParticleSystem* particleSystem, GPUEmitterTemplateHandle emitterTemplate, uint32_t maxParticles);
    ~GPUEmitter();

    GPUEmitter(GPUEmitter&&) = delete;
    GPUEmitter& operator=(GPUEmitter&&) = delete;

    GPUEmitter(const GPUEmitter&) = delete;
    GPUEmitter& operator=(const GPUEmitter&) = delete;

    GPUEmitter& SetSpawnRate(float spawnRate);
    GPUEmitter& SetParticleLifeTime(float lifeTime);
    GPUEmitter& SetParticleColor(const XMFLOAT4& color);
    GPUEmitter& SetPosition(const XMFLOAT3& position);

    inline const EmitterConstantData& GetConstantData() const { return mConstantData; }
    inline const EmitterStatusData GetDefaultStatusData() const { return EmitterStatusData{ mInitialSeed }; }

    inline bool GetEnabled() const { return mEnabled; }
    inline void SetEnabled(bool value) { mEnabled = value; }

    inline bool GetDirty() const { return mDirty; }
    inline void SetDitry() { mDirty = true; }
    inline void ClearDirty() { mDirty = false; }

    inline void SetTemplateHandle(GPUEmitterTemplateHandle handle) { mTemplateHandle = handle; }
    inline GPUEmitterTemplateHandle GetTemplateHandle() const { return mTemplateHandle; }

    inline Range GetParticleAllocation() const { return mParticleAllocation; }

    inline uint32_t GetEmitterIndexGPU() const { return GetIndex(); }
    inline uint32_t GetMaxParticles() const { return mConstantData.MaxParticles; }

private:
    GPUEmitterTemplateHandle mTemplateHandle;
    GPUParticleSystem* mParticleSystem = nullptr;

    EmitterConstantData mConstantData;

    Range mParticleAllocation;

    bool mDirty = true;
    bool mEnabled = true;

    uint32_t mInitialSeed = 0;
};

using GPUEmitterHandle = ObjectHandle<GPUEmitter>;

