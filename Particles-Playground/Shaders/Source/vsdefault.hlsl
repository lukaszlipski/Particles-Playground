#include "default.hlsli"

ConstantBuffer<VSContants> Constants : register(b0, space0);

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = float4(input.pos + Constants.x, 1);
    
	return output;
}