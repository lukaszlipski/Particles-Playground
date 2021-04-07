#pragma once
#include "Utilities\allocatorcommon.h"

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
};

struct EmitterStatusData
{
    uint32_t AliveParticles = 0;
    uint32_t ParticlesToSpawn = 0;
    uint32_t ParticlesToUpdate = 0;
    float SpawnAccTime = 0;
};

class GPUParticleSystem;
class CommandList;

class GPUEmitter
{
public:
    GPUEmitter() = default;
    ~GPUEmitter();

    GPUEmitter(GPUEmitter&&) = default;
    GPUEmitter& operator=(GPUEmitter&&) = default;

    GPUEmitter(const GPUEmitter&) = delete;
    GPUEmitter& operator=(const GPUEmitter&) = delete;

    void AllocateResources(uint32_t maxParticles, GPUParticleSystem& particleSystem);
    void FreeResources(GPUParticleSystem& particleSystem);

    GPUEmitter& SetSpawnRate(float spawnRate);
    GPUEmitter& SetParticleLifeTime(float lifeTime);
    GPUEmitter& SetParticleColor(const XMFLOAT4& color);

    inline const EmitterConstantData& GetConstantData() const { return mConstantData; }

    inline bool GetEnabled() const { return mEnabled; }
    inline void SetEnabled(bool value) { mEnabled = value; }

    inline bool GetDirty() const { return mDirty; }
    inline void SetDitry() { mDirty = true; }
    inline void ClearDirty() { mDirty = false; }

    inline Range& GetEmitterAllocation() { return mEmitterAllocation; }
    inline Range& GetParticleAllocation() { return mParticleAllocation; }

    inline uint32_t GetEmitterIndexGPU() const { return static_cast<uint32_t>(mEmitterAllocation.Start); }
    inline uint32_t GetMaxParticles() const { return mConstantData.MaxParticles; }

private:
    EmitterConstantData mConstantData;

    Range mEmitterAllocation;
    Range mParticleAllocation;

    bool mDirty = true;
    bool mEnabled = true;
};

using GPUEmitterHandle = GPUEmitter*;
