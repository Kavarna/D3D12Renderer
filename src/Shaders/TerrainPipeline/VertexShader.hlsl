#include "Common.hlsli"


VSOut main(VSIn input)
{
    VSOut output;
    
    output.Position = input.Position;
    
    return output;
}
