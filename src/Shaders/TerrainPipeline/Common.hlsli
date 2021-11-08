#ifndef _COMMON_HLSLI_
#define _COMMON_HLSLI_

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
    
    int HasTexture;
};

cbuffer SceneLights : register(b3)
{
    float4 AmbientColor;

    Light Lights[MAX_LIGHTS];
    
    unsigned int NumDirectionalLights;
    unsigned int NumPointLights;
    unsigned int NumSpotLights;
};

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};


struct VSIn
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
};

struct VSOut
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
};

struct HullOut
{
    float4 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
};

struct DomainOut
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
};


Texture2D diffuseMap : register(s0);

SamplerState wrapLinearSampler : register(s0);
SamplerState wrapPointSampler : register(s1);
SamplerState clampLinearSampler : register(s2);
SamplerState clampPointSampler : register(s3);


#endif // _COMMON_HLSLI_