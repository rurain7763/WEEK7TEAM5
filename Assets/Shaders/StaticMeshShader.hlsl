#define PI 3.14159265359

/*
#define LIGHTING_MODEL_GOURAUD (0U)
#define LIGHTING_MODEL_LAMBERT (1U)
#define LIGHTING_MODEL_BLINN_PHONG (2U)
*/


//#define LIGHTING_MODEL_GOURAUD (1U)
#define LIGHTING_MODEL_LAMBERT (1U)
//#define LIGHTING_MODEL_BLINN_PHONG (1U)

#define LIGHTING_MODEL (LIGHTING_MODEL_LAMBERT)

struct LightInfo
{
    float3 position;
    int type; // 0: directional, 1: point, 2: spot
    float4 color;
    float3 direction;
    float range;
    float intensity;
    float falloff;
    float inner_cone_angle;
    float outer_cone_angle;
};

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
    //float view_constants_padding;
    int IsWorldNormalViewMode;
}

cbuffer GlobalLightConstants : register(b2) 
{
    float4 ambient_color;
    float3 dl_directional;
    float ambient_intensity;
    float4 dl_color;
    float dl_intensity;
    float3 global_light_constants_padding;
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
StructuredBuffer<LightInfo> lights : register(t1);

SamplerState default_sampler : register(s0);

void calculate_directional(float3 N, float3 V, float3 light_direction, float3 p_light_color, float light_intensity, out float3 diffuse, out float3 specular)
{
    float3 light_color = p_light_color * light_intensity;
    float3 L = normalize(-light_direction);
    float NdotL = max(dot(N, L), 0.0);

    diffuse = light_color * NdotL;
    specular = float3(0.0, 0.0, 0.0);
    
#if LIGHTING_MODEL_BLINN_PHONG
    if (NdotL > 0.0)
    {
        float3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        specular = light_color * pow(NdotH, 32.0);
    }
#endif
}

void calculate_point(float3 N, float3 V, LightInfo light, float3 world_position, out float3 diffuse, out float3 specular)
{
    float3 light_color = light.color.rgb * light.intensity;
    
    float3 to_light = light.position - world_position;
    float dist = length(to_light);
    float3 L = to_light / dist;
    float NdotL = max(dot(N, L), 0.0);
    
    float fade = saturate(1.0 - dist / light.range);
    float attenuation = pow(fade, light.falloff);
    
    diffuse = light_color * NdotL * attenuation;
    specular = float3(0.0, 0.0, 0.0);
    
#if LIGHTING_MODEL_BLINN_PHONG
    if (NdotL > 0.0)
    {
        float3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        specular = light_color * pow(NdotH, 32.0) * attenuation;
    }
#endif
}

void calculate_spot(float3 N, float3 V, LightInfo light, float3 world_position, out float3 diffuse, out float3 specular)
{
    float3 light_color = light.color.rgb * light.intensity;
    
    float3 light_to_point = world_position - light.position;
    float dist = length(light_to_point);
    light_to_point /= dist;
    float3 light_forward = normalize(light.direction);
    float cos_theta = dot(light_forward, light_to_point);
    float inner_cos = cos(light.inner_cone_angle);
    float outer_cos = cos(light.outer_cone_angle);
    
    float spot_factor = saturate((cos_theta - outer_cos) / (inner_cos - outer_cos));
    
    float NdotL = max(dot(N, -light_to_point), 0.0);
    
    float fade = saturate(1.0 - dist / light.range);
    float attenuation = pow(fade, light.falloff);
    
    diffuse = light_color * NdotL * spot_factor * attenuation;
    specular = float3(0.0, 0.0, 0.0);
    
#if LIGHTING_MODEL_BLINN_PHONG
    if (NdotL > 0.0)
    {
        float3 H = normalize(-light_to_point + V);
        float NdotH = max(dot(N, H), 0.0);
        specular = light_color * pow(NdotH, 32.0) * spot_factor * attenuation;
    }
#endif
}

void shade(float3 world_position, float3 world_normal, inout float4 out_color)
{
    float3 V = normalize(view_position - world_position);
    
    float3 ambient = ambient_color.rgb * ambient_intensity;
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);

    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    calculate_directional(world_normal, V, dl_directional, dl_color.rgb, dl_intensity, diffuse, specular);
    
    total_diffuse += diffuse;
    total_specular += specular;
    
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];

        if (light.type == 1)
        {
            calculate_point(world_normal, V, light, world_position.xyz, diffuse, specular);
        }
        else if (light.type == 2)
        {
            calculate_spot(world_normal, V, light, world_position.xyz, diffuse, specular);
        }
        
        total_diffuse += diffuse;
        total_specular += specular;
    }
    
    out_color.rgb = out_color.rgb * (ambient + total_diffuse) + total_specular;
}

// Vertex Shader
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 world_position = mul(input.position, Model);
    float3 N = normalize(mul(input.normal, transpose((float3x3)inv_model)));    
    float4 output_color = float4(0.0, 0.0, 0.0, 1.0);
    if (UseVertexColor)
    {
        output_color = input.color;
    }
    else
    {
        output_color = Color;
    }
    
#if LIGHTING_MODEL_GOURAUD // LIGHTING_MODEL_GOURAUD
    shade(world_position.xyz, N, output_color);
#endif
    
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
    float3 world_normal = normalize(input.normal);
    float4 output_color = input.color;
    
    if (IsWorldNormalViewMode == 1)
    {
        output_color = float4((world_normal * 0.5 + 0.5), 1.0f);
        
        return output_color;
    }
    
    if (HasTexture != 0)
    {
        output_color *= main_texture.Sample(default_sampler, input.uv);
    }
    
#if LIGHTING_MODEL_GOURAUD 
    // Do nothing
#else 
    shade(input.world_position, world_normal, output_color);
#endif
    
    return output_color;
}
