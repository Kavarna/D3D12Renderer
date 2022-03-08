struct PerFrame
{
    float4 Colors[3];
};


RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);
ConstantBuffer<PerFrame> gPerFrame : register(b0);

struct RayPayload
{
    float3 color;
};

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDims = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDims.xy);

    float2 d = (crd / dims) * 2.0f - 1.0f;
    float aspectRatio = dims.x / dims.y;

    RayDesc ray;
    ray.Origin = float3(0.0f, 0.0f, -3.0f);
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));
    ray.TMin = 0.0f;
    ray.TMax = 10000.0f;

    RayPayload payload;
    TraceRay(gRtScene, 0, 0xFF, 0, 0, 0, ray, payload);

    gOutput[launchIndex.xy] = float4(payload.color, 1.0f);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = float3(0.0f, 0.0f, 0.0f);
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 barycentrics = float3(
        1.0f - attribs.barycentrics.x - attribs.barycentrics.y,
        attribs.barycentrics.x,
        attribs.barycentrics.y
    );

    uint instanceID = InstanceID();

    float3 A = float3(1.0f, 0.0f, 0.0f);
    float3 B = float3(0.0f, 1.0f, 0.0f);
    float3 C = float3(0.0f, 0.0f, 1.0f);

    payload.color = gPerFrame.Colors[instanceID % 3].xyz;
}
