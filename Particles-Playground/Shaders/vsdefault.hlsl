#include "default.hlsli"

struct VSContants
{
    uint indicesOffset;
};

struct SceneCB
{
    float4x4 proj;
    float4x4 view;
};

ConstantBuffer<VSContants> Constants : register(b0, space0);
StructuredBuffer<SceneCB> Camera : register(t0, space0);
StructuredBuffer<ParticlesData> Data : register(t1, space0);
StructuredBuffer<int> Indices : register(t2, space0);

VSOutput main(VSInput input)
{
    VSOutput output;
    
    int index = Indices[Constants.indicesOffset + input.id];
    ParticlesData data = Data[Constants.indicesOffset + index];
    float4x4 mat = mul(Camera[0].proj, Camera[0].view);
    
    output.pos = mul(mat, float4((input.pos * data.scale) + data.position, 1));
    output.color = data.color;
    output.texCoord = input.texCoord;
    
	return output;
}