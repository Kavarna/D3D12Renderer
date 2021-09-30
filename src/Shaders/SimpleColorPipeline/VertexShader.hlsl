#include "Common.hlsli"


VSOut main(in VSIn input)
{
    VSOut output;
    
    float4x4 VP = mul(View, Projection);
    
    output.PositionW = mul(float4(input.Position, 1.0f), World).xyz;
    output.Position = mul(float4(output.PositionW, 1.0f), VP);
    
    output.Color = input.Color;

    return output;
}

