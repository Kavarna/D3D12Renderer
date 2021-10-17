#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__


struct VSIn
{
    float3 Position : POSITION;
    float2 TexCoords : TEXCOORD;
};

struct VSOut
{
    float4 Position : SV_POSITION;
    float2 TexCoords : COLOR;
};

Texture2D diffuseMap : register(s0);

SamplerState wrapLinearSampler : register(s0);
SamplerState wrapPointSampler : register(s1);
SamplerState clampLinearSampler : register(s2);
SamplerState clampPointSampler : register(s3);

#endif // __COMMON_HLSLI__