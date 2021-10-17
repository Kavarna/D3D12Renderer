#include "Common.hlsli"


VSOut main(in VSIn input)
{
    VSOut output;
    output.Position = float4(input.Position, 1.0f);
    
    output.TexCoords = input.TexCoords;

    return output;
}

