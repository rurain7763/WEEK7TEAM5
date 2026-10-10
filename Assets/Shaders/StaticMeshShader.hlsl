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
    int padding[2];
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
StructuredBuffer<LightInfo> lights : register(t1);

SamplerState default_sampler : register(s0);

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
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];
        
        float3 light_color = light.color.rgb * light.intensity;
        
        float3 diffuse = float3(0.0, 0.0, 0.0);
        if (light.type == 0)
        {
            float NdotL = max(dot(N, -light.direction), 0.0);
            
            diffuse = light_color * NdotL;
        }
        else if (light.type == 1)
        {
            float3 to_light = light.position - world_position.xyz;
            float dist = length(to_light);
            float3 L = to_light / dist;
            float NdotL = max(dot(N, L), 0.0);
            
            float fade = saturate(1.0 - dist / light.range);
            float attenuation = pow(fade, light.falloff);
            
            diffuse = light_color * NdotL * attenuation;
        }
        else if (light.type == 2)
        {
            float3 light_to_point = world_position.xyz - light.position;
            float dist = length(light_to_point);
            light_to_point /= dist;
            float3 light_forward = normalize(light.direction);
            float cos_theta = dot(light_forward, light_to_point);
            float inner_cos = cos(radians(light.inner_cone_angle));
            float outer_cos = cos(radians(light.outer_cone_angle));
            
            float spot_factor = saturate((cos_theta - outer_cos) / (inner_cos - outer_cos));
            float fade = saturate(1.0 - dist / light.range);
            float attenuation = pow(fade, light.falloff);
            
            diffuse = light_color * spot_factor * attenuation;
        }
        
        total_diffuse += diffuse;
    }
    
    output_color.rgb *= (ambient_color.rgb * ambient_intensity + total_diffuse);
#else
#endif
    
    output.position = mul(world_position, view_projection);
    output.uv = input.uv + uv_offset;
    output.color = output_color;
    output.normal = N;
    output.world_position = world_position;
    
	return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
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
        
        float3 light_color = light.color.rgb * light.intensity;
        
        float3 diffuse = float3(0.0, 0.0, 0.0);
        float3 specular = float3(0.0, 0.0, 0.0);
        if (light.type == 0)
        {
            float NdotL = max(dot(N, -light.direction), 0.0);
            
            diffuse = light_color * NdotL;
            
            float3 R = reflect(light.direction, N);
            float RdotV = max(dot(R, V), 0.0);
            
            specular = light_color * pow(RdotV, 32.0) * NdotL;
        }
        else if (light.type == 1)
        {
            float3 to_light = light.position - input.world_position.xyz;
            float dist = length(to_light);
            float3 L = to_light / dist;
            float NdotL = max(dot(N, L), 0.0);
            
            float fade = saturate(1.0 - dist / light.range);
            float attenuation = pow(fade, light.falloff);
            
            diffuse = light_color * NdotL * attenuation;
            
            float3 R = reflect(-L, N);
            float RdotV = max(dot(R, V), 0.0);
            
            specular = light_color * pow(RdotV, 32.0) * NdotL * attenuation;
        }
        else if (light.type == 2)
        {
            float3 light_to_point = input.world_position.xyz - light.position;
            float dist = length(light_to_point);
            light_to_point /= dist;
            float3 light_forward = normalize(light.direction);
            float cos_theta = dot(light_forward, light_to_point);
            float inner_cos = cos(radians(light.inner_cone_angle));
            float outer_cos = cos(radians(light.outer_cone_angle));
            
            float spot_factor = saturate((cos_theta - outer_cos) / (inner_cos - outer_cos));
            
            float NdotL = max(dot(N, -light_to_point), 0.0);
            
            float fade = saturate(1.0 - dist / light.range);
            float attenuation = pow(fade, light.falloff);
            
            diffuse = light_color * NdotL * spot_factor * attenuation;
            
            float3 R = reflect(light_to_point, N);
            float RdotV = max(dot(R, V), 0.0);
            
            specular = light_color * pow(RdotV, 32.0) * NdotL * spot_factor * attenuation;
        }
        
        total_diffuse += diffuse;
        total_specular += specular;
    }
    
    output_color.rgb *= (ambient + total_diffuse + total_specular);
#endif
    
    return output_color;
}
