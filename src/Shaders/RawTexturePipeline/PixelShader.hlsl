#include "Common.hlsli"


float4 main(VSOut input) : SV_TARGET
{
    return diffuseMap.Sample(wrapLinearSampler, input.TexCoords);
}

