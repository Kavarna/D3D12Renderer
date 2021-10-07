#include "Common.hlsli"


VSOut main(in VSIn input)
{
    VSOut output;
    
    float4x4 VP = mul(View, Projection);
    
    output.PositionW = mul(float4(input.Position, 1.0f), World).xyz;
    output.Position = mul(float4(output.PositionW, 1.0f), VP);
    
    output.NormalW = mul(input.Normal, (float3x3) World);
    
    float4 texC = mul(float4(input.TexCoord, 0.0f, 1.0f), TexTransform);
    output.TexCoord = mul(texC, MatTransform).xy;

    return output;
}

