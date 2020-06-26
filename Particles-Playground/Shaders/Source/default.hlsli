
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
