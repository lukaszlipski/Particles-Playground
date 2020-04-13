#include "default.hlsli"

ConstantBuffer<VSContants> Constants : register(b0, space0);
StructuredBuffer<ParticleData> Data : register(t0, space0);

VSOutput main(VSInput input)
{
    VSOutput output;
    
    ParticleData data = Data[input.id];
    float4x4 mat = mul(Constants.proj, Constants.view);
    
    output.pos = mul(mat, float4(input.pos + data.position, 1));
    output.texCoord = input.texCoord;
    output.radius = data.radius;
    output.fade = data.fade;
    
	return output;
}