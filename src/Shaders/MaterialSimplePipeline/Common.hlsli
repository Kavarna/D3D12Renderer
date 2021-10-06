#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define MAX_LIGHTS 15

cbuffer cbPerObject : register(b0)
{
    float4x4 World;
    float4x4 TexTransform;
};

cbuffer cbPerFrame : register(b1)
{
    float4x4 View;
    float4x4 Projection;
};

cbuffer cbMaterial : register(b2)
{
    float4 DiffuseAlbedo;

    float3 FresnelR0;
    float Roughness;

    float4x4 MatTransform;
    
    unsigned int HasTexture;
};


struct VSIn
{
    float3 Position : POSITION;
};

struct VSOut
{
    float4 Position : SV_POSITION;
    float3 PositionW : POSITION;
    float4 Color : COLOR;
};

#endif // __COMMON_HLSLI__