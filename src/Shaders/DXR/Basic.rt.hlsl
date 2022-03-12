struct PerInstance
{
    float4 Color;
};


RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);
ConstantBuffer<PerInstance> gPerInstance : register(b0);

struct RayPayload
{
    float3 color;
};

struct ShadowPayload
{
    float3 multiplier;
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

    payload.color = gPerInstance.Color.xyz;
    // payload.color = float3(1.0f, 1.0f, 0.0f);
}

[shader("closesthit")]
void chs1(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 color = float3(1.f, 1.f, 1.f);
    
    float3 rayOrigin = WorldRayOrigin();
    float3 rayDirection = WorldRayDirection();
    float3 rayT = RayTCurrent();

    float3 posW = rayOrigin + rayT * rayDirection;

    RayDesc ray;
    ray.Origin = posW;
    ray.Direction = normalize(float3(0.0f, 1.0f, 0.0f));
    ray.TMin = 0.01f;
    ray.TMax = 10000.0f;

    ShadowPayload shadowPayload;
    TraceRay(gRtScene, 0  /*rayFlags*/, 0xFF, 1 /* ray index*/, 0, 1, ray, shadowPayload);
    payload.color = color * shadowPayload.multiplier;
    // payload.color = color;
}


[shader("miss")]
void shadowMiss(inout ShadowPayload payload)
{
    payload.multiplier = float3(1.0f, 1.0f, 1.0f);
}

[shader("closesthit")]
void shadowClosestHit(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.multiplier = float3(0.5f, 0.5f, 0.5f);
}