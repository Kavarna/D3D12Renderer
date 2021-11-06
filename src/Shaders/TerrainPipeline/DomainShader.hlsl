#include "Common.hlsli"



[domain("quad")]
DomainOut main(PatchTess pt, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;
    
    float4 v1 = lerp(quad[0].Position, quad[1].Position, uv.x);
    float4 v2 = lerp(quad[2].Position, quad[3].Position, uv.x);
    float4 p = lerp(v1, v2, uv.y);
    
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
    
    float4 PosW = mul(p, World);
    float4x4 ViewProj = mul(View, Projection);
    
    dout.Position = mul(PosW, ViewProj);
    
    return dout;
}
