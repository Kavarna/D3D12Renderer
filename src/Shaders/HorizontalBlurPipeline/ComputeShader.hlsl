
static const int gMaxBlurRadius = 5;
struct BufferInfo
{
    int blurRadius;
    
    // Support up to 11 blur weights.
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

ConstantBuffer<BufferInfo> cb0 : register(b0);
Texture2D inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

#define NUMTHREADX 256

groupshared float4 gCache[NUMTHREADX + 2 * gMaxBlurRadius];
[numthreads(NUMTHREADX, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID )
{
    float weights[11] = { cb0.w0, cb0.w1, cb0.w2, cb0.w3, cb0.w4, cb0.w5, cb0.w6, cb0.w7, cb0.w8, cb0.w9, cb0.w10 };
    uint width, height, numberOfLevels;
    inputTexture.GetDimensions(0, width, height, numberOfLevels);
 
    if (GTid.x < cb0.blurRadius)
    {
        int x = max((int) DTid.x - cb0.blurRadius, 0);
        gCache[GTid.x] = inputTexture[int2(x, DTid.y)];
    }
    else if (GTid.x >= NUMTHREADX - cb0.blurRadius)
    {
        int x = min(DTid.x + cb0.blurRadius, width - 1);
        gCache[GTid.x + 2 * cb0.blurRadius] = inputTexture[int2(x, DTid.y)];
    }
    
    gCache[GTid.x + cb0.blurRadius] = inputTexture[min(DTid.xy, uint2(width - 1, height - 1))];
    
    GroupMemoryBarrierWithGroupSync();
    
    float4 blurColor = 0.0f;
    for (int i = -cb0.blurRadius; i <= cb0.blurRadius; ++i)
    {
        int k = GTid.x + cb0.blurRadius + i;
        blurColor += weights[i + cb0.blurRadius] * gCache[k];
    }

    outputTexture[DTid.xy] = blurColor;
}