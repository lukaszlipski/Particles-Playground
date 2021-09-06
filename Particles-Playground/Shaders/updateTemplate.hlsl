#include "default.hlsli"

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

    if (id.x >= emitterConstant.maxParticles)
    {
        return;
    }

    uint offset = emitterConstant.indicesOffset;
    ParticlesData particle = Particles[offset + id.x];

    if (particle.lifeTime > 0)
    {
        // Update logic
        {
            TOKEN_UPDATE_LOGIC
        }

        Particles[offset + id.x] = particle;

        if (particle.lifeTime <= 0)
        {
            int freeListIndex;
            InterlockedAdd(EmitterStatus[emitterIndex].aliveParticles, -1, freeListIndex);
            freeListIndex -= 1;

            FreeList[offset + freeListIndex] = id.x;
        }
        else
        {
            int index;
            InterlockedAdd(DrawIndirectArgs[emitterIndex].instanceCount, 1, index);
            Indices[offset + index] = id.x;
        }

    }

}
