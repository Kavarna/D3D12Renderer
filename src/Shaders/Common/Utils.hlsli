#ifndef _UTILS_HLSLI_
#define _UTILS_HLSLI_


#define MAX_LIGHTS 10


struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // spot / directional only
    float FalloffEnd; // point/spot light only
    float3 Position; // spot / point light only
    float SpotPower; // Spotlight only
};


float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));
    
    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
    
    return reflectPercent;
}


float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);
    
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), 0) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);
    
    float3 specAlbedo = fresnelFactor * roughnessFactor;
    
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
    
    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float CalcAttenuation(float d, float attStart, float attEnd)
{
    return saturate((attEnd - d) / (attEnd - attStart));
}

float3 ComputeDirectionalLight(in float3 toEye, in float3 Normal, in Material mat, in Light l)
{
    float3 lightVec = -l.Direction;
    
    float NdotL = max(dot(lightVec, Normal), 0.0f);
    float3 lightStrength = l.Strength * NdotL;
    
    return BlinnPhong(lightStrength, lightVec, Normal, toEye, mat);
}

float3 ComputePointLight(in Light light, in Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 toLight = light.Position - pos;
    
    float d = length(toLight);
    
    if (d > light.FalloffEnd)
        return 0.0f;
    
    toLight /= d;
    
    float NdotL = max(dot(toLight, normal), 0.0f);
    float3 lightStrength = light.Strength * NdotL;
    
    float att = CalcAttenuation(d, light.FalloffStart, light.FalloffEnd);
    lightStrength *= att;
    
    return BlinnPhong(lightStrength, toLight, normal, toEye, mat);
}

float3 ComputeSpotlight(in Light light, in Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 toLight = light.Position - pos;
    
    float d = length(toLight);
    
    if (d > light.FalloffEnd)
        return 0.0f;
    
    toLight /= d;
    
    float NdotL = max(dot(toLight, normal), 0.0f);
    float3 lightStrength = light.Strength * NdotL;
    
    float att = CalcAttenuation(d, light.FalloffStart, light.FalloffEnd);
    lightStrength *= att;
    
    float spotFactor = pow(max(dot(-toLight, light.Direction), 0.0f), light.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, toLight, normal, toEye, mat);
}

float4 ComputeLighting(Light lights[MAX_LIGHTS], int numDirectionalLights, int numPointLights, int numSpotlights,
                       Material mat, float3 pos, float3 normal, float3 toEye)
{
    int i;
    float3 result = 0.0f;
    
    for (i = 0; i < numDirectionalLights; ++i)
    {
        result += ComputeDirectionalLight(toEye, normal, mat, lights[i]);
    }
    
    for (i = numDirectionalLights; i < numDirectionalLights + numPointLights; ++i)
    {
        result += ComputePointLight(lights[i], mat, pos, normal, toEye);
    }
    
    for (i = numDirectionalLights + numPointLights; i < numDirectionalLights + numPointLights + numSpotlights; ++i)
    {
        result += ComputeSpotlight(lights[i], mat, pos, normal, toEye);

    }

    return float4(result, 0.0f);
}

#define BILINEAR_INTERPOLATION_IMPLEMENTATION(type) \
type BilinearInterpolation(type params[4], float2 interpolationParams) \
{ \
    type v1 = lerp(params[0], params[1], interpolationParams.x); \
    type v2 = lerp(params[2], params[3], interpolationParams.x); \
    return lerp(v1, v2, interpolationParams.y); \
}


BILINEAR_INTERPOLATION_IMPLEMENTATION(float1)
BILINEAR_INTERPOLATION_IMPLEMENTATION(float2)
BILINEAR_INTERPOLATION_IMPLEMENTATION(float3)
BILINEAR_INTERPOLATION_IMPLEMENTATION(float4)

#endif // _UTILS_HLSLI_

