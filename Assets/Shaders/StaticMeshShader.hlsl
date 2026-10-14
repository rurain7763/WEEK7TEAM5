#define PI 3.14159265359

#define TEXTURE_MAP_BASE_COLOR 1 << 0
#define TEXTURE_MAP_SPECULAR 1 << 1
#define TEXTURE_MAP_NORMAL 1 << 2

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
    uint binded_texture_map_flags;
    int light_count;
    float shininess;
    float2 padding;
}

cbuffer ViewConstants : register(b1) // FConstants
{
    row_major matrix view_projection;
    float3 view_position;
    float view_constants_padding;
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
    float4 tangent : TANGENT;
};

#if LIGHTING_MODEL_GOURAUD
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float3 diffuse : COLOR1;
    float3 specular : COLOR2;
    float2 uv : TEXCOORD0;
};
#else
struct PS_INPUT
{
	float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
};
#endif

struct PS_OUTPUT
{
    float4 color : SV_TARGET0;
    float3 normal : SV_TARGET1;
};

Texture2D base_color_texture : register(t0);
Texture2D specular_texture : register(t1);
Texture2D normal_texture : register(t2);
StructuredBuffer<LightInfo> lights : register(t3);

SamplerState default_sampler : register(s0);

//안전하게 정규화
float3 SafeNormalize(float3 v)
{
    //dot(v,v) : 자기 자신을 내적 : 벡터 길이의 제곱(length^2)
    //rsqrt : 역 제곱근 : 1/루트x
    //v / 길이로 하기 위해 제곱을 구한 dot 결과를 역제곱근한다.
    return v * rsqrt(max(dot(v, v), 1e-12));
}

void calculate_diretional(float3 N, float3 V, float3 light_direction, float3 p_light_color, float light_intensity, float light_shininess, out float3 diffuse, out float3 specular)
{
    float3 light_color = p_light_color * light_intensity;
    
    float3 L = SafeNormalize(-light_direction);
    float NdotL = max(dot(N, L), 0.0);

    diffuse = light_color * NdotL;
    specular = float3(0.0, 0.0, 0.0);

#if LIGHTING_MODEL_BLINN_PHONG
    if (NdotL > 0.0)
    {
        float3 H = SafeNormalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        specular = light_color * pow(NdotH, light_shininess);
    }
#endif
}

void calculate_point(float3 N, float3 V, LightInfo light, float3 world_position, float light_shininess, out float3 diffuse, out float3 specular)
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
        float3 H = SafeNormalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        specular = light_color * pow(NdotH, light_shininess) * attenuation;
    }
#endif
}

void calculate_spot(float3 N, float3 V, LightInfo light, float3 world_position, float light_shininess, out float3 diffuse, out float3 specular)
{
    float3 light_color = light.color.rgb * light.intensity;
    
    float3 light_to_point = world_position - light.position;
    float dist = length(light_to_point);
    light_to_point /= dist;
    float3 light_forward = SafeNormalize(light.direction);
    float cos_theta = dot(light_forward, light_to_point);
    float inner_cos = cos(light.inner_cone_angle);
    float outer_cos = cos(light.outer_cone_angle);
    
    float spot_factor = saturate((cos_theta - outer_cos) / max((inner_cos - outer_cos), 1e-6));
    
    float NdotL = max(dot(N, -light_to_point), 0.0);
    
    float fade = saturate(1.0 - dist / light.range);
    float attenuation = pow(fade, light.falloff);
    
    diffuse = light_color * NdotL * spot_factor * attenuation;
    specular = float3(0.0, 0.0, 0.0);
    
#if LIGHTING_MODEL_BLINN_PHONG
    if (NdotL > 0.0)
    {
        float3 H = SafeNormalize(-light_to_point + V);
        float NdotH = max(dot(N, H), 0.0);
        specular = light_color * pow(NdotH, light_shininess) * spot_factor * attenuation;
    }
#endif
}

// Vertex Shader
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    float4 world_position = mul(input.position, Model);
    float3 N = SafeNormalize(mul(input.normal, transpose((float3x3) inv_model)));
    
    float4 output_color = float4(0.0, 0.0, 0.0, 1.0);
    if (UseVertexColor)
    {
        output_color = input.color;
    }
    else
    {
        output_color = Color;
    }
    
#if LIGHTING_MODEL_GOURAUD
    float3 V = SafeNormalize(view_position - world_position.xyz);
    
    float3 ambient = ambient_color.rgb * ambient_intensity;
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);

    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    calculate_diretional(N, V, dl_directional, dl_color.rgb, dl_intensity, shininess, diffuse, specular);
    
    total_diffuse += diffuse;
    total_specular += specular;
    
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];

        if (light.type == 1)
        {
            calculate_point(N, V, light, world_position.xyz, shininess, diffuse, specular);
        }
        else if (light.type == 2)
        {
            calculate_spot(N, V, light, world_position.xyz, shininess, diffuse, specular);
        }
        
        total_diffuse += diffuse;
        total_specular += specular;
    }

#if ENABLE_LIGHTING    
    output.diffuse = total_diffuse;
    output.specular = total_specular;
#endif
#else
    float4 T = float4(SafeNormalize(mul(input.tangent.xyz, (float3x3) Model)), input.tangent.w);

    output.tangent = T;
    output.world_position = world_position.xyz;
#endif
    
    output.position = mul(world_position, view_projection);
    output.normal = N;
    output.uv = input.uv + uv_offset;
    output.color = output_color;
        
    return output;
}

// Pixel Shader
PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float4 output_color = input.color;
    float3 ambient = ambient_color.rgb * ambient_intensity;
    float3 N = SafeNormalize(input.normal);
    
#if LIGHTING_MODEL_GOURAUD
    if (binded_texture_map_flags & TEXTURE_MAP_BASE_COLOR)
    {
        output_color *= base_color_texture.Sample(default_sampler, input.uv);
    }
    
    float3 specular = input.specular;
    if (binded_texture_map_flags & TEXTURE_MAP_SPECULAR)
    {
        specular *= specular_texture.Sample(default_sampler, input.uv).rgb;
    }
    
    output_color.rgb = output_color.rgb * (ambient + input.diffuse) + specular;
#else
    if (binded_texture_map_flags & TEXTURE_MAP_BASE_COLOR)
    {
        output_color *= base_color_texture.Sample(default_sampler, input.uv);
    }
    
    if (binded_texture_map_flags & TEXTURE_MAP_NORMAL)
    {
        // 넘어온 tangent가 normal과 직교하지 않을 수 있으므로, Gram-Schmidt orthogonalize를 이용하여 tangent을 정규화하고, normal과 직교하도록 수정합니다.
        float3 T = SafeNormalize(input.tangent.xyz - dot(N, input.tangent.xyz) * N);
        float3 B = cross(N, T) * input.tangent.w;
        float3x3 TBN = float3x3(T, B, N);
        
        float3 normal_map = normal_texture.Sample(default_sampler, input.uv).xyz;
        normal_map = normal_map * 2.0 - 1.0; // Transform from [0,1] to [-1,1]
        
        N = SafeNormalize(mul(normal_map, TBN));
    }
    
    float3 V = SafeNormalize(view_position - input.world_position);
    
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);

    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    calculate_diretional(N, V, dl_directional, dl_color.rgb, dl_intensity, shininess, diffuse, specular);
    
    total_diffuse += diffuse;
    total_specular += specular;
    
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];        

        if (light.type == 1)
        {
            calculate_point(N, V, light, input.world_position, shininess, diffuse, specular);
        }
        else if (light.type == 2)
        {
            calculate_spot(N, V, light, input.world_position, shininess, diffuse, specular);
        }
        
        total_diffuse += diffuse;
        total_specular += specular;
    }
    
    if (binded_texture_map_flags & TEXTURE_MAP_SPECULAR)
    {
        total_specular *= specular_texture.Sample(default_sampler, input.uv).rgb;
    }
    
#if ENABLE_LIGHTING
    output_color.rgb = output_color.rgb * (ambient + total_diffuse) + total_specular;
#endif
#endif
        
    output.color = output_color;
    output.normal = N;
    
    return output;
}