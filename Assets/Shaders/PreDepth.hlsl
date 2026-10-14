cbuffer ModelConstants : register(b0)
{
    row_major matrix Model;
}

cbuffer ViewConstants : register(b1) 
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
    float4 tangent : TANGENT;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    float4 world_position = mul(input.position, Model);
    output.position = mul(world_position, view_projection);
    
    return output;
}
