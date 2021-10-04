#include "default.hlsli"

static uint Internal_RandomSeed = 0;
static uint Internal_ParticleIndex = 0;
void Internal_InitRandom(uint emitterSeed, uint particleIndex)
{
    Internal_RandomSeed = emitterSeed;
    Internal_ParticleIndex = particleIndex;
}

uint GetRandom()
{
    Internal_RandomSeed = GetRandomXxHash32(uint2(Internal_RandomSeed, Internal_ParticleIndex));
    return Internal_RandomSeed;
}

float GetRandomFloat()
{
    uint random = GetRandom();
    random &= 0x007FFFFFU; // Extract mantisa part
    random |= 0x3F800000U; // Set exponent to 127, this will result in float [1;2)
    return asfloat(random) - 1.0f;
}
