#include "default.hlsli"

ConstantBuffer<PSContants> Constants : register(b1, space0);

float4 main(VSOutput input) : SV_TARGET
{
    const float delta = 0.04f;
    float dist = distance(input.texCoord, float2(0.5f, 0.5f));
    float val = smoothstep(input.radius - delta, input.radius + delta, dist);
    float3 color = float3(1.0f, Constants.x, 1.0f) * input.fade;
    return float4(color, 1.0f - val);
}