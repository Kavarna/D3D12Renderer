#include "Common.hlsli"


float4 main(VSOut input) : SV_TARGET
{
    input.NormalW = normalize(input.NormalW);
    
    float4 diffuseColor;
    if (HasTexture != -1)
    {
        diffuseColor = diffuseMap.Sample(clampLinearSampler, input.TexCoord);
    }
    else
    {
        diffuseColor = DiffuseAlbedo;
    }
    
    float3 toEyeW = CameraPosition - input.PositionW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;
        
    float4 finalColor = AmbientColor * diffuseColor;
    
    Material mat;
    mat.DiffuseAlbedo = diffuseColor;
    mat.FresnelR0 = FresnelR0;
    mat.Shininess = Shininess;
    float4 directLight = ComputeLighting(Lights, NumDirectionalLights, NumPointLights, NumSpotLights, mat, input.PositionW.xyz, input.NormalW, toEyeW);
    
    finalColor += directLight;
    finalColor.a = diffuseColor.a;
    
    return finalColor;
}

