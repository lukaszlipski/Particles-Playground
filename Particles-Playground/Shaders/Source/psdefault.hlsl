#include "default.hlsli"

struct PSContants
{
    float x;
};

float4 main(VSOutput input) : SV_TARGET
{
    const float delta = 0.04f;
    float dist = distance(input.texCoord, float2(0.5f, 0.5f));
    float val = smoothstep(input.radius - delta, input.radius + delta, dist);
    float alpha = (1.0f - val) * input.color.a;
    return float4(input.color.rgb, alpha);
}