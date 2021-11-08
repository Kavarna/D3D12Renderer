#include "Common.hlsli"

//float4 BilinearInterpolation(float4 params[4], float2 interpolationParams)
//{
//    float4 v1 = lerp(params[0], params[1], interpolationParams.x);
//    float4 v2 = lerp(params[2], params[3], interpolationParams.x);
//    return lerp(v1, v2, interpolationParams.y);
//}

static float Epsilon = 0.0001f;


float HeightFunction(float x, float z)
{
    return 0.3f * (z * sin(x) + x * cos(z));
}

[domain("quad")]
DomainOut main(PatchTess pt, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;
    
    float4 positions[4] = { quad[0].Position, quad[1].Position, quad[2].Position, quad[3].Position };
    float4 p = BilinearInterpolation(positions, uv);
    p.y = HeightFunction(p.x, p.z);
    
    float4 PosW = mul(p, World);
    float4x4 ViewProj = mul(View, Projection);
    
    dout.Position = mul(PosW, ViewProj);
    
    
    float3 p1 = float3(p.x + Epsilon, HeightFunction(p.x + Epsilon, p.z), p.z);
    float3 p2 = float3(p.x, HeightFunction(p.x, p.z + Epsilon), p.z + Epsilon);
    float3 upVector = normalize(p1 - p.xyz);
    float3 rightVector = normalize(p2 - p.xyz);
    float3 normal = normalize(cross(rightVector, upVector));
    dout.Normal = mul(normal, (float3x3) World);
    
    float2 texCoords[4] = { quad[0].TexCoords, quad[1].TexCoords, quad[2].TexCoords, quad[3].TexCoords };
    float2 texCoord = BilinearInterpolation(texCoords, uv);
    float4 texC = mul(float4(texCoord, 0.0f, 1.0f), TexTransform);
    dout.TexCoords = mul(texC, MatTransform).xy;
    
    return dout;
}
