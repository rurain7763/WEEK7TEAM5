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
    
    int light_count;
    int UseWorldNormalView;
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

void calculate_diretional(float3 N, float3 V, float3 light_direction, float3 p_light_color, float light_intensity, out float3 diffuse, out float3 specular)
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
    
    float spot_factor = saturate((cos_theta - outer_cos) / max(inner_cos - outer_cos, 1e-4));
    
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
    
#if 0  // LIGHTING_MODEL_GOURAUD
    float3 V = normalize(view_position - world_position.xyz);
    
    float3 ambient = ambient_color.rgb * ambient_intensity;
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);

    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    calculate_diretional(N, V, dl_directional, dl_color.rgb, dl_intensity, diffuse, specular);
    
    total_diffuse += diffuse;
    total_specular += specular;
    
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];        

        if (light.type == 1)
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
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 output_color = float4(0.0, 0.0, 0.0, 1.0);
    
    if (UseWorldNormalView)
    {
        //픽셀 셰이더에 들어온 보간된 Normal은 길이가 1이 아닐 수 있기에 normalize 해준다.
        float3 NormalizeNormal = normalize(input.normal);
        //normal을 색으로 변환 
        //-1 ~ 1 -> 0.0 ~ 1.0
        //감마보정을 대비해 2.2승
        output_color.rgb = pow((NormalizeNormal + 1.0) * 0.5, 2.2);
        
        //아래의 식대로 출력하면 본래 월드 노멀보다 조금 밝은 색이 나온다.
        //이유는 백버퍼에 쓸 때 감마보정이 들어가기 때문
        //2.2승을 하여 출력하면 본래의 노멀 값대로 색상이 적용된다.
        //하지만 노멀 색상을 2.2승하여 출력하게 되면 이 노멀 값을 가지고 계산하게 될 때 틀린 값을 사용하게 된다.
        //지금 당장 계산된 노멀값을 쓰고 있진 않지만 차후 노멀값이 기록된 버퍼를 만들어야 할 때 2.2승을 하면 안된다.
        //output_color.rgb = (NormalizeNormal + 1.0) * 0.5;
    }
    else
    {
        output_color = input.color;
        
#if 0 //LIGHTING_MODEL_GOURAUD
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
    float3 V = normalize(view_position - input.world_position);
    
    float3 ambient = ambient_color.rgb * ambient_intensity;
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);

    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    calculate_diretional(N, V, dl_directional, dl_color.rgb, dl_intensity, diffuse, specular);
    
    total_diffuse += diffuse;
    total_specular += specular;
    
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];        

        if (light.type == 1)
        {
            calculate_point(N, V, light, input.world_position, diffuse, specular);
        }
        else if (light.type == 2)
        {
            calculate_spot(N, V, light, input.world_position, diffuse, specular);
        }
        
        total_diffuse += diffuse;
        total_specular += specular;
    }
    
    output_color.rgb = output_color.rgb * (ambient + total_diffuse) + total_specular;
#endif
    }
    

    
    return output_color;
}
