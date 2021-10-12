#include "default.hlsli"

struct VSContants
{
    uint indicesOffset;
};

struct CameraCB
{
    float4x4 proj;
    float4x4 view;
};

ConstantBuffer<VSContants> Constants : register(b0, space0);
ConstantBuffer<CameraCB> Camera : register(b1, space0);
StructuredBuffer<ParticlesData> Data : register(t0, space0);
StructuredBuffer<int> Indices : register(t1, space0);

VSOutput main(VSInput input)
{
    VSOutput output;
    
    int index = Indices[Constants.indicesOffset + input.id];
    ParticlesData data = Data[Constants.indicesOffset + index];
    float4x4 mat = mul(Camera.proj, Camera.view);
    
    output.pos = mul(mat, float4((input.pos * data.scale) + data.position, 1));
    output.color = data.color;
    output.texCoord = input.texCoord;
    
	return output;
}