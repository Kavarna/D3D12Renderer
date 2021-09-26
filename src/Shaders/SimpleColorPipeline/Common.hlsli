#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

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