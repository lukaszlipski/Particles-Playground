
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
    float3 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
};
