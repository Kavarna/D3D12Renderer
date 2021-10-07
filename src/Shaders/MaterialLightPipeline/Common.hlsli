#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#include "../Common/Utils.hlsli"

cbuffer cbPerObject : register(b0)
{
    float4x4 World;
    float4x4 TexTransform;
};

cbuffer cbPerFrame : register(b1)
{
    float4x4 View;
    float4x4 Projection;
    
    float3 CameraPosition;
};

cbuffer cbMaterial : register(b2)
{
    float4 DiffuseAlbedo;

    float3 FresnelR0;
    float Shininess;

    float4x4 MatTransform;
    
    unsigned int HasTexture;
};

cbuffer SceneLights : register(b3)
{
    float4 AmbientColor;

    Light Lights[MAX_LIGHTS];
    
    unsigned int NumDirectionalLights;
    unsigned int NumPointLights;
    unsigned int NumSpotLights;
};

struct VSIn
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VSOut
{
    float4 Position : SV_POSITION;
    float3 PositionW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoord : TEXCOORD;
};

#endif // __COMMON_HLSLI__
