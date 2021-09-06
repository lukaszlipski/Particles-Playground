#include "screen.hlsli"

Texture2D<float4> Texture : register(t0, space0);
SamplerState Sampler : register(s0, space0);

float4 main(VSOutput input) : SV_TARGET
{
	float3 color = Texture.Sample(Sampler, input.texCoord).rgb;
	float3 gammaCorrected = pow(color, 1.0f/2.2f);
	return float4(gammaCorrected, 1.0f);
}
