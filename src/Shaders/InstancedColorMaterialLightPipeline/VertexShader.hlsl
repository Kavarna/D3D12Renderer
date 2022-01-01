#include "Common.hlsli"


VSOut main(in VSIn input, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
    VSOut output;

    float4x4 VP = mul(View, Projection);

    float4x4 World = instanceData[instanceID].World;
    output.Color = instanceData[instanceID].Color;

    output.PositionW = mul(float4(input.Position, 1.0f), World).xyz;

    [branch]
    if (vertexID % 2 == 0)
    {
        output.PositionW += input.Normal * instanceData[instanceID].AnimationTime;
    }
    else
    {
        output.PositionW -= input.Normal * instanceData[instanceID].AnimationTime;
    }

    output.Position = mul(float4(output.PositionW, 1.0f), VP);

    output.NormalW = mul(input.Normal, (float3x3) World);

    output.TexCoord = input.TexCoord;

    return output;
}

