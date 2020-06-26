#include "default.hlsli"

struct VSContants
{
    float4x4 proj;
    float4x4 view;
};

ConstantBuffer<VSContants> Constants : register(b0, space0);
StructuredBuffer<ParticlesData> Data : register(t0, space0);
StructuredBuffer<int> Indices : register(t1, space0);

VSOutput main(VSInput input)
{
    VSOutput output;
    
    int index = Indices[input.id];
    ParticlesData data = Data[index];
    float4x4 mat = mul(Constants.proj, Constants.view);
    
    output.pos = mul(mat, float4(input.pos + data.position, 1));
    output.color = data.color;
    output.texCoord = input.texCoord;
    output.radius = lerp(0.1f, 0.4f, data.scale);
    output.fade = data.scale;
    
	return output;
}