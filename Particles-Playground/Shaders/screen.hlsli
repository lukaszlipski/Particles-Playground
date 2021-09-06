
struct VSInput
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};
