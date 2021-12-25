#include "Common.hlsli"


VSOut main(in VSIn input, uint instanceID : SV_InstanceID)
{
    VSOut output;

    float4x4 VP = mul(View, Projection);

    output.Color = input.Color;
    output.Position = mul(float4(input.Position, 1.0f), VP);

    return output;
}

