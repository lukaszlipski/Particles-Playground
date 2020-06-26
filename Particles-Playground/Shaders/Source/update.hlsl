#include "default.hlsli"

struct UpdateConstants
{
    float deltaTime;
};

ConstantBuffer<UpdateConstants> Constants : register(b0, space0);
RWStructuredBuffer<ParticlesData> Particles : register(u0, space0);
RWStructuredBuffer<EmitterData> Emitter : register(u1, space0);
RWStructuredBuffer<int> Indices : register(u2, space0);
RWStructuredBuffer<int> FreeList : register(u3, space0);
RWStructuredBuffer<DrawIndirectArgs> DrawIndirectArgs : register(u4, space0);

[numthreads(64, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
    if (id.x >= Emitter[0].maxParticles)
    {
        return;
    }

    ParticlesData particle = Particles[id.x];

    if (particle.lifeTime > 0)
    {
        particle.position += particle.velocity * Constants.deltaTime;
        particle.velocity += float3(0, -9.8f, 0) * Constants.deltaTime;
        //particle.scale;
        particle.color.a = max(0.0f, particle.lifeTime / Emitter[0].lifeTime);

        particle.lifeTime -= Constants.deltaTime;

        Particles[id.x] = particle;

        if (particle.lifeTime <= 0)
        {
            int freeListIndex;
            InterlockedAdd(Emitter[0].aliveParticles, -1, freeListIndex);
            freeListIndex -= 1;

            FreeList[freeListIndex] = id.x;
        }
        else
        {
            int index;
            InterlockedAdd(DrawIndirectArgs[0].instanceCount, 1, index);
            Indices[index] = id.x;
        }
    }
    
}
