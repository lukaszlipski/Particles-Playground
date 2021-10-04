
struct VSInput
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    uint id : SV_InstanceID;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
    float radius : RADIUS;
    float fade : FADE;
};

struct DrawIndirectArgs
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
};

struct DispatchIndirectArgs
{
    uint threadGroupCountX;
    uint threadGroupCountY;
    uint threadGroupCountZ;
};

struct ParticlesData
{
    float3 position;
    float lifeTime;
    float3 velocity;
    float scale;
    float4 color;
};

struct EmitterData
{
    uint aliveParticles;
    uint maxParticles;
    uint particlesToSpawn;
    float spawnAccTime;
    float spawnRate;
    float lifeTime;
};

struct EmitterConstantData
{
    uint maxParticles;
    float spawnRate;
    float lifeTime;
    uint indicesOffset;
    float4 color;
    float3 position;
};

struct EmitterStatusData
{
    uint currentSeed;
    uint aliveParticles;
    uint particlesToSpawn;
    uint particlesToUpdate;
    float spawnAccTime;
};

uint GetRandomPCG(uint seed)
{
    uint state = seed * 747796405U + 2891336453U;
    uint word = ((state >> ((state >> 28U) + 4U)) ^ state) * 277803737U;
    return (word >> 22U) ^ word;
}

uint GetRandomXxHash32(uint2 seed)
{
    const uint PRIME32_2 = 2246822519U;
    const uint PRIME32_3 = 3266489917U;
    const uint PRIME32_4 = 668265263U;
    const uint PRIME32_5 = 374761393U;

    uint h32 = seed.y + PRIME32_5 + seed.x * PRIME32_3;
    h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
    h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}
