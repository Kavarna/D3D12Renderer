#include "Common.hlsli"


VSOut main(in VSIn input, uint instanceID : SV_InstanceID)
{
    VSOut output;

    float4x4 VP = mul(View, Projection);

    float4x4 World = instanceData[instanceID].World;
    output.Color = instanceData[instanceID].Color;

    output.PositionW = mul(float4(input.Position, 1.0f), World).xyz;
    output.Position = mul(float4(output.PositionW, 1.0f), VP);

    output.NormalW = mul(input.Normal, (float3x3) World);

    output.TexCoord = input.TexCoord;

    return output;
}

