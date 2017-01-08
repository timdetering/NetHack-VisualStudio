// Per-pixel color data passed through the pixel shader.
Texture2D AsciiTexture : register(t0);
SamplerState AsciiSampler : register(s0);

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 foregroundColor : COLOR0;
    float3 backgroundColor : COLOR1;
    float2 tex : TEXCOORD0;
};

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 foregroundSample = AsciiTexture.Sample(AsciiSampler, input.tex);

    if (foregroundSample.x == 0.0)
    {
        return float4(input.backgroundColor, 0.0f);
    }
    else
    {
        return foregroundSample * float4(input.foregroundColor, 0.0f);
    }
}
