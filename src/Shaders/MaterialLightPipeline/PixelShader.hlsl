#include "Common.hlsli"


float4 main(VSOut input) : SV_TARGET
{
    input.NormalW = normalize(input.NormalW);
    
    float4 diffuseColor;
    if (HasTexture != -1)
    {
        diffuseColor = float4(1.0f, 1.0f, 0.0f, 0.0f);
    }
    else
    {
        diffuseColor = DiffuseAlbedo;
    }
    
    float3 toEyeW = CameraPosition - input.PositionW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;
    
    float4 finalColor = diffuseColor;
    
    return finalColor;
}

