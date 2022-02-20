
RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    float3 col = float3(0.4f, 0.6f, 0.2f); // Background color
    gOutput[launchIndex.xy] = float4(col, 1.0f);
}

struct Payload
{
    bool hit;
};

[shader("miss")]
void miss(inout Payload p)
{
    p.hit = false;
}

[shader("closesthit")]
void chs(inout Payload p, in BuiltInTriangleIntersectionAttributes attribs)
{
    p.hit = true;
}
