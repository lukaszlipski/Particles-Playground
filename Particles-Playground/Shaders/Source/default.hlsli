
struct VSContants
{
    float x;
};

struct PSContants
{
    float x;
};

struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
};
