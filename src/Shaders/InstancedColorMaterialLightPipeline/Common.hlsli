#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#include "../Common/Utils.hlsli"

cbuffer cbPerFrame : register(b0)
{
    float4x4 View;
    float4x4 Projection;

    float3 CameraPosition;
};

cbuffer cbMaterial : register(b1)
{
    float4 DiffuseAlbedo;

    float3 FresnelR0;
    float Shininess;

    float4x4 MatTransform;

    int HasTexture;
};

cbuffer SceneLights : register(b2)
{
    float4 AmbientColor;

    Light Lights[MAX_LIGHTS];

    unsigned int NumDirectionalLights;
    unsigned int NumPointLights;
    unsigned int NumSpotLights;
};

struct InstanceInfo
{
    row_major float4x4 World;
    float4 Color;

    float AnimationTime;
    float3 pad;
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
    float4 Color : COLOR;
};

StructuredBuffer<InstanceInfo> instanceData : register(t0, space1);

Texture2D diffuseMap : register(t0);

SamplerState wrapLinearSampler : register(s0);
SamplerState wrapPointSampler : register(s1);
SamplerState clampLinearSampler : register(s2);
SamplerState clampPointSampler : register(s3);

#endif // __COMMON_HLSLI__
