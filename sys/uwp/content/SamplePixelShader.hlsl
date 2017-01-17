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
    float4 alpha = AsciiTexture.Sample(AsciiSampler, input.tex);
    float4 inverseAlpha = float4(1.0f, 1.0f, 1.0f, 0.0f) - alpha;

    return (alpha * float4(input.foregroundColor, 0.0f)) + (inverseAlpha * float4(input.backgroundColor, 0.0f));
}
