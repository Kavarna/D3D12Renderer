
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

#define NUMTHREADY 256

groupshared float4 gCache[NUMTHREADY + 2 * gMaxBlurRadius];
[numthreads(1, NUMTHREADY, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID )
{
    float weights[11] = { cb0.w0, cb0.w1, cb0.w2, cb0.w3, cb0.w4, cb0.w5, cb0.w6, cb0.w7, cb0.w8, cb0.w9, cb0.w10 };
    uint width, height, numberOfLevels;
    inputTexture.GetDimensions(0, width, height, numberOfLevels);
    
    if (GTid.y < cb0.blurRadius)
    {
        int y = max(DTid.y - cb0.blurRadius, 0);
        gCache[GTid.y] = inputTexture[int2(DTid.x, y)];
    }
    else if (GTid.y >= NUMTHREADY - cb0.blurRadius)
    {
        int y = min(DTid.y + cb0.blurRadius, height - 1);
        gCache[GTid.y + 2 * cb0.blurRadius] = inputTexture[int2(DTid.x, y)];
    }
    
    gCache[GTid.y + cb0.blurRadius] = inputTexture[min(DTid.xy, uint2(width - 1, height - 1))];

    GroupMemoryBarrierWithGroupSync();
    
    float4 blurColor = 0.0f;
    for (int i = -cb0.blurRadius; i <= cb0.blurRadius; ++i)
    {
        int k = GTid.y + cb0.blurRadius + i;
        blurColor += weights[i + cb0.blurRadius] * gCache[ k];
    }

    outputTexture[DTid.xy] = blurColor;
}