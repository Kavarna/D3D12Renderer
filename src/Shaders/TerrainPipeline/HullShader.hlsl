#include "Common.hlsli"


PatchTess ConstantHS(InputPatch<VSOut, 4> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    float3 centerL = 0.25f * (patch[0].Position + patch[1].Position + patch[2].Position + patch[3].Position).xyz;
    float3 centerW = mul(float4(centerL, 1.0f), World).xyz;
    
    float d = distance(centerW, CameraPosition);
    
    const float d0 = 10.0f;
    const float d1 = 200.0f;
    float tess = 64.f * saturate((d1 - d) / (d1 - d0));
    
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;

    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;
    
    return pt;
}

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut main(InputPatch<VSOut, 4> p,
           uint i : SV_OutputControlPointID,
           uint patchId : SV_PrimitiveID)
{
    HullOut hout;
	
    hout.Position = p[i].Position;
    hout.Normal = p[i].Normal;
    hout.TexCoords = p[i].TexCoords;
	
    return hout;
}

