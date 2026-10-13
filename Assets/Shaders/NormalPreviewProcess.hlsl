struct PS_INPUT
{
	float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D normal_texture : register(t0);

SamplerState default_sampler : register(s0);

// Vertex Shader
PS_INPUT mainVS(uint vertex_id : SV_VertexID)
{
    PS_INPUT output;
	
    float2 full_screen_quad[4] =
    {
        float2(-1.0, 1.0),
		float2(1.0, 1.0),
		float2(1.0, -1.0),
		float2(-1.0, -1.0)
    };
    
    float2 uv_coords[4] =
    {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(1.0, 1.0),
        float2(0.0, 1.0)
    };
    
    uint indices[] = { 0, 1, 2, 0, 2, 3 }; // Two triangles to form a quad
	
    output.position = float4(full_screen_quad[indices[vertex_id]], 0.0, 1.0);
    output.uv = uv_coords[indices[vertex_id]];

    return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 normal = normal_texture.Sample(default_sampler, input.uv).rgb;
    normal = (normal + 1.0) * 0.5; // Convert from [-1, 1] to [0, 1]
    
    return float4(normal, 1.0);
}
