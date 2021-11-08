#include "Common.hlsli"


VSOut main(VSIn input)
{
    VSOut output;
    
    output.Position = float4(input.Position, 1.0f);
    output.Normal = input.Normal;
    output.TexCoords = input.TexCoords;
    
    return output;
}
