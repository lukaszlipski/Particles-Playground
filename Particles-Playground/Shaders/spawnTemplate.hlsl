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
RWStructuredBuffer<ParticlesData> Particles : register(u0, space0);
RWStructuredBuffer<uint> FreeList : register(u1, space0);
RWStructuredBuffer<uint> Indices : register(u2, space0);
RWStructuredBuffer<DrawIndirectArgs> DrawIndirectArgs : register(u3, space0);
RWStructuredBuffer<EmitterStatusData> EmitterStatus : register(u4, space0);

groupshared uint FreeListStartIndex;
groupshared uint InstanceStartIndex;

[numthreads(64, 1, 1)]
void main(SpawnInput input)
{
    uint emitterIndex = Constants.emitterIndex;
    uint spawnIndex = input.globalThreadID.x;
    uint spawnGroupIndex = input.groupThreadID.x;

    if (spawnIndex >= EmitterStatus[emitterIndex].particlesToSpawn)
    {
        return;
    }

    EmitterConstantData emitterConstant = EmitterConstant[emitterIndex];

    // Allocate indicies for new particles and slots for alive instances
    if (input.groupThreadID.x == 0)
    {
        uint groupParticlesCount = min(64 * (input.groupID.x + 1), EmitterStatus[emitterIndex].particlesToSpawn) - (input.groupID.x * 64);
        InterlockedAdd(EmitterStatus[emitterIndex].freeListPointer, groupParticlesCount, FreeListStartIndex);
        InterlockedAdd(DrawIndirectArgs[emitterIndex].instanceCount, groupParticlesCount, InstanceStartIndex);
    }
    GroupMemoryBarrierWithGroupSync();

    uint offset = emitterConstant.indicesOffset;

    // Get index for current particle
    uint freeListOffset = offset + FreeListStartIndex + spawnGroupIndex;
    uint particleIndex = FreeList[freeListOffset];
    FreeList[freeListOffset] = -1;

    // Setup instance index for current particle
    uint instanceOffset = offset + InstanceStartIndex + spawnGroupIndex;
    Indices[instanceOffset] = particleIndex;

    Internal_InitRandom(EmitterStatus[emitterIndex].currentSeed, particleIndex);

    ParticlesData particle;

    // Spawn logic
    {
        TOKEN_SPAWN_LOGIC
    }

    Particles[offset + particleIndex] = particle;
}
