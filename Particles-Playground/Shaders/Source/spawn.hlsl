#include "default.hlsli"

struct SpawnConstants
{
    uint emitterIndex;
};

struct SpawnInput
{
    uint3 globalThreadID : SV_DispatchThreadID;
    uint3 groupThreadID : SV_GroupThreadID;
    uint3 groupID : SV_GroupID;
};

ConstantBuffer<SpawnConstants> Constants : register(b0, space0);
StructuredBuffer<EmitterConstantData> EmitterConstant : register(t0, space0);
RWStructuredBuffer<ParticlesData> Particles : register(u0, space0);
RWStructuredBuffer<EmitterStatusData> EmitterStatus : register(u1, space0);
RWStructuredBuffer<uint> FreeList : register(u2, space0);

groupshared int ParticleGroupIndex;

[numthreads(64, 1, 1)]
void main( SpawnInput input )
{
    if (input.groupThreadID.x == 0)
    {
        ParticleGroupIndex = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint emitterIndex = Constants.emitterIndex;
    if (input.globalThreadID.x >= EmitterStatus[emitterIndex].particlesToSpawn)
    {
        return;
    }

    EmitterConstantData emitterConstant = EmitterConstant[emitterIndex];

    uint spawnIndex;
    InterlockedAdd(ParticleGroupIndex, 1, spawnIndex);
    spawnIndex += input.groupID.x * 64;

    uint offset = emitterConstant.indicesOffset;
    uint freeListIndex = offset + spawnIndex + (EmitterStatus[emitterIndex].aliveParticles - EmitterStatus[emitterIndex].particlesToSpawn);
    int particleIndex = FreeList[freeListIndex];
    FreeList[freeListIndex] = -1;

    ParticlesData particle;
    
    // Spawn logic
    {
        float phi = (float(particleIndex) / emitterConstant.maxParticles) * 3.14f;

        particle.position = float3(0.0f, 0.0f, 0.0f);
        particle.color = emitterConstant.color;
        particle.lifeTime = emitterConstant.lifeTime;
        particle.velocity = float3(cos(phi), sin(phi), 0) * 15.0f;
        particle.scale = 1.0f;
    }

    Particles[offset + particleIndex] = particle;
}
