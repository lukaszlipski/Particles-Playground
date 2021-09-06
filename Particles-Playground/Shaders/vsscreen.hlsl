#include "screen.hlsli"

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = float4(input.pos, 1.0f);
    output.texCoord = input.texCoord;
    
    return output;
}
