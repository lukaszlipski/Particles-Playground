#include "particlecommon.hlsli"

struct UpdateConstants
{
    uint emitterIndex;
    float deltaTime;
};

ConstantBuffer<UpdateConstants> Constants : register(b0, space0);
StructuredBuffer<EmitterConstantData> EmitterConstant : register(t0, space0);
RWStructuredBuffer<ParticlesData> Particles : register(u0, space0);
RWStructuredBuffer<EmitterStatusData> EmitterStatus : register(u1, space0);
RWStructuredBuffer<uint> Indices : register(u2, space0);
RWStructuredBuffer<uint> FreeList : register(u3, space0);
RWStructuredBuffer<DrawIndirectArgs> DrawIndirectArgs : register(u4, space0);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint emitterIndex = Constants.emitterIndex;
    EmitterConstantData emitterConstant = EmitterConstant[emitterIndex];

    uint particleIndex = id.x;
    if (particleIndex >= emitterConstant.maxParticles)
    {
        return;
    }

    uint offset = emitterConstant.indicesOffset;
    ParticlesData particle = Particles[offset + particleIndex];

    if (particle.lifeTime > 0)
    {
        Internal_InitRandom(EmitterStatus[emitterIndex].currentSeed, particleIndex);

        // Update logic
        {
            TOKEN_UPDATE_LOGIC
        }

        Particles[offset + particleIndex] = particle;

        if (particle.lifeTime <= 0)
        {
            uint freeListIndex;
            InterlockedAdd(EmitterStatus[emitterIndex].freeListPointer, -1, freeListIndex);
            freeListIndex -= 1;

            FreeList[offset + freeListIndex] = particleIndex;
        }
        else
        {
            int index;
            InterlockedAdd(DrawIndirectArgs[emitterIndex].instanceCount, 1, index);
            Indices[offset + index] = particleIndex;
        }

    }

}
