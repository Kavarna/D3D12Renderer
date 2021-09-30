#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__


cbuffer WVP : register(b0)
{
    float4x4 World;
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
    float3 PositionW : POSITION;
    float4 Color : COLOR;
};

#endif // __COMMON_HLSLI__