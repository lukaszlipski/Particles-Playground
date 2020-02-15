#include "default.hlsli"

ConstantBuffer<PSContants> Constants : register(b1, space0);

float4 main() : SV_TARGET
{
    return float4(1.0f, Constants.x, 1.0f, 1.0f);
}