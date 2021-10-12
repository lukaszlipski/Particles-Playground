#include "default.hlsli"

struct EmitterUpdateConstants
{
    uint emittersCount;
    float deltaTime;
};

ConstantBuffer<EmitterUpdateConstants> Constants : register(b0, space0);
StructuredBuffer<EmitterConstantData> EmitterConstant : register(t0, space0);
StructuredBuffer<uint> EmitterIndexBuffer : register(t1, space0);
RWStructuredBuffer<EmitterStatusData> EmitterStatus : register(u1, space0);
RWStructuredBuffer<DrawIndirectArgs> DrawIndirectBuffer : register(u2, space0);
RWStructuredBuffer<DispatchIndirectArgs> SpawnIndirectBuffer : register(u3, space0);

[numthreads(64, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
    if (id.x >= Constants.emittersCount)
    {
        return;
    }

    // Indirection to emitter's contant and status buffer
    uint emitterIndex = EmitterIndexBuffer[id.x];

    // Update emitter data
    EmitterStatusData emitterStatus = EmitterStatus[emitterIndex];
    EmitterConstantData emitterConstant = EmitterConstant[emitterIndex];

    emitterStatus.updateTime += Constants.deltaTime;
    
    if (emitterConstant.loopTime == -1.0f || emitterStatus.updateTime <= emitterConstant.loopTime)
    {
        emitterStatus.spawnAccTime += Constants.deltaTime;

        // Update emitter's seed with PCG RNG
        emitterStatus.currentSeed = GetRandomPCG(emitterStatus.currentSeed);

        uint freeCount = EmitterConstant[emitterIndex].maxParticles - emitterStatus.aliveParticles;
        uint maxSpawnCount = floor(emitterStatus.spawnAccTime * EmitterConstant[emitterIndex].spawnRate);

        emitterStatus.particlesToSpawn = min(freeCount, maxSpawnCount);
        emitterStatus.spawnAccTime -= float(maxSpawnCount) / EmitterConstant[id.x].spawnRate;

        // Add the particles to spawn in advance so spawn shader doesn't have to
        emitterStatus.aliveParticles += emitterStatus.particlesToSpawn;
        emitterStatus.particlesToUpdate = emitterStatus.aliveParticles;
    }
    else
    {
        emitterStatus.particlesToSpawn = 0;
    }

    EmitterStatus[emitterIndex] = emitterStatus;

    // Preapre spawn indirect buffer
    uint spawnDispatchNum = (emitterStatus.particlesToSpawn + 63) / 64;
    SpawnIndirectBuffer[emitterIndex].threadGroupCountX = spawnDispatchNum;
    SpawnIndirectBuffer[emitterIndex].threadGroupCountY = 1;
    SpawnIndirectBuffer[emitterIndex].threadGroupCountZ = 1;

    // Reset draw indirect buffer
    DrawIndirectBuffer[emitterIndex].instanceCount = 0;
}
