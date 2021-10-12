#include "default.hlsli"

//Texture2D<float4> Texture : register(t2, space0);
SamplerState Sampler : register(s0, space0);

float4 main(VSOutput input) : SV_TARGET
{
    const float delta = 0.05f;
    const float radius = 0.45f;
    float dist = distance(input.texCoord, float2(0.5f, 0.5f));
    float val = smoothstep(radius - delta, radius + delta, dist);
    float alpha = (1.0f - val) * input.color.a;
    //float4 color = Texture.Sample(Sampler, input.texCoord);
    return float4(input.color.xyz, alpha);
}