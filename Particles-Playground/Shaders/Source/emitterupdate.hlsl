#include "default.hlsli"

struct EmitterUpdateConstants
{
    float deltaTime;
};

ConstantBuffer<EmitterUpdateConstants> Constants : register(b0, space0);
RWStructuredBuffer<EmitterData> Emitter : register(u0, space0);
RWStructuredBuffer<DrawIndirectArgs> DrawIndirectBuffer : register(u1, space0);
RWStructuredBuffer<DispatchIndirectArgs> DispatchIndirectBuffer : register(u2, space0);

[numthreads(1, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
    // Reset draw indirect buffer
    DrawIndirectBuffer[0].instanceCount = 0;

    // Update emitter data
    Emitter[id.x].spawnAccTime += Constants.deltaTime;

    uint freeCount = Emitter[id.x].maxParticles - Emitter[id.x].aliveParticles;
    uint maxSpawnCount = floor(Emitter[id.x].spawnAccTime * Emitter[id.x].spawnRate);

    Emitter[id.x].particlesToSpawn = min(freeCount, maxSpawnCount);
    Emitter[id.x].spawnAccTime -= float(maxSpawnCount) / Emitter[id.x].spawnRate;

    Emitter[id.x].aliveParticles += Emitter[id.x].particlesToSpawn; // Add the particles to spawn in advance so spawn shader doesn't have to

    // Preapre dispatch indirect buffer
    DispatchIndirectBuffer[0].threadGroupCountX = ceil(float(Emitter[id.x].particlesToSpawn) / 64.0f);
    DispatchIndirectBuffer[0].threadGroupCountY = 1;
    DispatchIndirectBuffer[0].threadGroupCountZ = 1;

}
