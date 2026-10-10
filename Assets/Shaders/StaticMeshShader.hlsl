#define PI 3.14159265359

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
    float4 ambient_color;
    float ambient_intensity;
    int light_count;
    int2 padding;
}

cbuffer ViewConstants : register(b1) // FConstants
{
	row_major matrix view_projection;
    float3 view_position;
    float view_constants_padding;
}

cbuffer AmbientConstants : register(b10) 
{
    float4 AmbientColor;
    float AmbientIntensity;
}

cbuffer DirectionalLightConstants : register(b11) 
{
    float3 DL_Direction;
    float DL_Intensity;
    float4 DL_Color;
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

struct PS_OUTPUT
{
    float4 color : SV_TARGET0;
    float3 normal : SV_TARGET1;
};

Texture2D main_texture : register(t0);
StructuredBuffer<LightInfo> lights : register(t1);

SamplerState default_sampler : register(s0);

void calculate_diretional(float3 N, float3 V, LightInfo light, out float3 diffuse, out float3 specular)
{
    float3 light_color = light.color.rgb * light.intensity;
    
    float3 L = normalize(-light.direction);
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
    
#if 1 // LIGHTING_MODEL_GOURAUD
    float3 V = normalize(view_position - world_position.xyz);
    
    float3 ambient = ambient_color.rgb * ambient_intensity;
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];        

        float3 diffuse = float3(0.0, 0.0, 0.0);
        float3 specular = float3(0.0, 0.0, 0.0);
        if (light.type == 0)
        {
            calculate_diretional(N, V, light, diffuse, specular);
        }
        else if (light.type == 1)
        {
            calculate_point(N, V, light, world_position.xyz, diffuse, specular);
        }
        else if (light.type == 2)
        {
            calculate_spot(N, V, light, world_position.xyz, diffuse, specular);
        }
        
        total_diffuse += diffuse;
        total_specular += specular;
    }
    

    output_color.rgb = output_color.rgb * (ambient + total_diffuse) + total_specular;
#endif
    
    output.position = mul(world_position, view_projection);
    output.uv = input.uv + uv_offset;
    output.color = output_color;
    output.normal = N;
    output.world_position = world_position.xyz;
    
	return output;
}

// Pixel Shader
PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float4 output_color = input.color;
    
#if 1 //LIGHTING_MODEL_GOURAUD
    if (HasTexture)
    {
        output_color *= main_texture.Sample(default_sampler, input.uv);
    }
#else
    if (HasTexture != 0)
    {
        output_color *= main_texture.Sample(default_sampler, input.uv);
    }
    
    float3 N = normalize(input.normal);
    float3 V = normalize(view_position - input.world_position.xyz);
	
    float3 ambient = ambient_color.rgb * ambient_intensity;
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];
        
        float3 diffuse = float3(0.0, 0.0, 0.0);
        float3 specular = float3(0.0, 0.0, 0.0);
        if (light.type == 0)
        {
            calculate_diretional(N, V, light, diffuse, specular);
        }
        else if (light.type == 1)
        {
            calculate_point(N, V, light, input.world_position.xyz, diffuse, specular);
        }
        else if (light.type == 2)
        {
            calculate_spot(N, V, light, input.world_position.xyz, diffuse, specular);
        }
        
        total_diffuse += diffuse;
        total_specular += specular;
    }
    
    output_color.rgb = output_color.rgb * (ambient + total_diffuse) + total_specular;
#endif
    
    output.color = output_color;
    output.normal = input.normal;
    
    return output;
}
