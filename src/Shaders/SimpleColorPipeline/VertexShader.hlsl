#include "Common.hlsli"


VSOut main(in VSIn input)
{
    VSOut output;
    
    output.Position = float4(input.Position, 1.0f);
    output.Color = input.Color;

    return output;
}

