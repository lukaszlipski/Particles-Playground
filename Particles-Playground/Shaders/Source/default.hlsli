
struct VSContants
{
    float4x4 proj;
    float4x4 view;
};

struct PSContants
{
    float x;
};

struct VSInput
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
    uint id : SV_InstanceID;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float radius : RADIUS;
    float fade : FADE;
};

struct ParticleData
{
    float3 position;
    float radius;
    float fade;
};
