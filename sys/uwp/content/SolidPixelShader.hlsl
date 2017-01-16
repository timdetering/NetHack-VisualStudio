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
	return float4(input.foregroundColor, 1.0f);
}
