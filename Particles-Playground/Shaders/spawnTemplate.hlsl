#include "particlecommon.hlsli"

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
StructuredBuffer<EmitterStatusData> EmitterStatus : register(t1, space0);
RWStructuredBuffer<ParticlesData> Particles : register(u0, space0);
RWStructuredBuffer<uint> FreeList : register(u1, space0);

groupshared int ParticleGroupIndex;

[numthreads(64, 1, 1)]
void main(SpawnInput input)
{
    if (input.groupThreadID.x == 0)
    {
        ParticleGroupIndex = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint emitterIndex = Constants.emitterIndex;
    EmitterStatusData emitterStatus = EmitterStatus[emitterIndex];

    if (input.globalThreadID.x >= emitterStatus.particlesToSpawn)
    {
        return;
    }

    EmitterConstantData emitterConstant = EmitterConstant[emitterIndex];

    uint spawnIndex;
    InterlockedAdd(ParticleGroupIndex, 1, spawnIndex);
    spawnIndex += input.groupID.x * 64;

    uint offset = emitterConstant.indicesOffset;
    uint freeListIndex = offset + spawnIndex + (emitterStatus.aliveParticles - emitterStatus.particlesToSpawn);
    int particleIndex = FreeList[freeListIndex];
    FreeList[freeListIndex] = -1;

    Internal_InitRandom(emitterStatus.currentSeed, particleIndex);

    ParticlesData particle;

    // Spawn logic
    {
        TOKEN_SPAWN_LOGIC
    }

    Particles[offset + particleIndex] = particle;
}
