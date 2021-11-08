#include "Common.hlsli"

float4 main(DomainOut input) : SV_Target
{
    input.Normal = normalize(input.Normal);
    // return abs(float4(input.Normal, 1.0f));
    
    float4 diffuseColor;
    if (HasTexture != -1)
    {
        diffuseColor = diffuseMap.Sample(clampLinearSampler, input.TexCoords);
    }
    else
    {
        diffuseColor = DiffuseAlbedo;
    }
    
    float3 toEyeW = CameraPosition - input.Position;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;
        
    float4 finalColor = AmbientColor * diffuseColor;
    
    Material mat;
    mat.DiffuseAlbedo = diffuseColor;
    mat.FresnelR0 = FresnelR0;
    mat.Shininess = Shininess;
    float4 directLight = ComputeLighting(Lights, NumDirectionalLights, NumPointLights, NumSpotLights, mat, input.Position.xyz, input.Normal, toEyeW);
    
    finalColor += directLight;
    finalColor.a = diffuseColor.a;
    
    return finalColor;
}