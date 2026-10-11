#define PI 3.14159265359

cbuffer ModelConstants : register(b0) // FConstants
{
    row_major matrix Model;
    row_major matrix inv_model;
    float4 Color;
    float2 uv_offset;
    int UseVertexColor;
    int HasTexture;
    int light_count;
    float3 padding;
}

cbuffer ViewConstants : register(b1) // FConstants
{
    row_major matrix view_projection;
    float3 view_position;
    float view_constants_padding;
}


struct VS_INPUT
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
};

Texture2D main_texture : register(t0);

SamplerState default_sampler : register(s0);

// Vertex Shader
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 world_position = mul(input.position, Model);
    float3 N = normalize(mul(input.normal, transpose((float3x3) inv_model)));
    float4 output_color = float4(0.0, 0.0, 0.0, 1.0);

    output_color.rgb = (output_color.rgb + N)*0.5+0.5;
    output.position = mul(world_position, view_projection);
    output.uv = input.uv + uv_offset;
    output.color = output_color;
    output.normal = N;
    output.world_position = world_position.xyz;
    
    return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 output_color = input.color;
    
    if (HasTexture)
    {
        output_color *= main_texture.Sample(default_sampler, input.uv);
    }
    
    return output_color;
}
