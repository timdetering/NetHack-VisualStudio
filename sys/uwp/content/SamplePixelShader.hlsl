// Per-pixel color data passed through the pixel shader.
Texture2D AsciiTexture : register(t0);
SamplerState AsciiSampler : register(s0);

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
    float2 tex : TEXCOORD0;
};

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
    return AsciiTexture.Sample(AsciiSampler, input.tex) * float4(input.color, 0.0f);
//	return float4(input.color, 1.0f);
}
