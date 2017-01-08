// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
    float3 pos : POSITION;
    float3 foregroundColor : COLOR0;
    float3 backgroundColor : COLOR1;
    float2 tex : TEXCOORD0;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 foregroundColor : COLOR0;
    float3 backgroundColor : COLOR1;
    float2 tex : TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
    PixelShaderInput output;
    float4 pos = float4(input.pos, 1.0f);

    output.pos = pos;

    // Pass the color through without modification.
    output.foregroundColor = input.foregroundColor;
    output.backgroundColor = input.backgroundColor;
    output.tex = input.tex;

    return output;
}
