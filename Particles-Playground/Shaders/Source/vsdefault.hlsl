#include "default.hlsli"

ConstantBuffer<VSContants> Constants : register(b0, space0);

VSOutput main(VSInput input)
{
    VSOutput output;
    
    float4x4 mat = mul(Constants.proj, Constants.view);
    
    output.pos = mul(mat, float4(input.pos, 1));
    
	return output;
}