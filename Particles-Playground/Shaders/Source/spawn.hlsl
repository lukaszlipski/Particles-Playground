#include "default.hlsli"

struct SpawnInput
{
    uint3 globalThreadID : SV_DispatchThreadID;
    uint3 groupThreadID : SV_GroupThreadID;
    uint3 groupID : SV_GroupID;
};

RWStructuredBuffer<ParticlesData> Particles : register(u0, space0);
RWStructuredBuffer<EmitterData> Emitter : register(u1, space0);
RWStructuredBuffer<int> FreeList : register(u2, space0);

groupshared int ParticleGroupIndex;

[numthreads(64, 1, 1)]
void main( SpawnInput input )
{
    if (input.groupThreadID.x == 0)
    {
        ParticleGroupIndex = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    if (input.globalThreadID.x >= Emitter[0].particlesToSpawn)
    {
        return;
    }

    uint spawnIndex;
    InterlockedAdd(ParticleGroupIndex, 1, spawnIndex);
    spawnIndex += input.groupID.x * 64;

    uint freeListIndex = spawnIndex + (Emitter[0].aliveParticles - Emitter[0].particlesToSpawn);

    int particleIndex = FreeList[freeListIndex];
    FreeList[freeListIndex] = -1;

    // Spawn logic
    ParticlesData particle;

    float phi = (float(particleIndex) / Emitter[0].maxParticles) * 3.14f;

    particle.position = float3(0.0f, 0.0f, 0.0f);
    particle.color = float4(float(particleIndex) / Emitter[0].maxParticles, 0.0f, 1.0f, 1.0f);
    particle.lifeTime = Emitter[0].lifeTime;
    particle.velocity = float3(cos(phi), sin(phi), 0) * 15.0f;
    particle.scale = 1.0f;

    Particles[particleIndex] = particle;
}
