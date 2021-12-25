#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#include "../Common/Utils.hlsli"

cbuffer cbPerFrame : register(b0)
{
    float4x4 View;
    float4x4 Projection;
};

struct VSIn
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VSOut
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

#endif // __COMMON_HLSLI__
