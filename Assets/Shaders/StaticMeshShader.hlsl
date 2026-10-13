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
    
    float shininess;
    float3 lightpadding;
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
    float4 tangent : TANGENT;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
    float4 tangent : TANGENT;
};

//머티리얼 텍스처 t0 ~ t7
Texture2D main_texture : register(t0);
Texture2D normal_texture : register(t1);
//전역 리소스 t8 ~
StructuredBuffer<LightInfo> lights : register(t8);

SamplerState default_sampler : register(s0);

//안전하게 정규화
float3 SafeNormalize(float3 v)
{
    //dot(v,v) : 자기 자신을 내적 : 벡터 길이의 제곱(length^2)
    //rsqrt : 역 제곱근 : 1/루트x
    //v / 길이로 하기 위해 제곱을 구한 dot 결과를 역제곱근한다.
    return v * rsqrt(max(dot(v, v), 1e-12));
}

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
    float3 L = dist > 0.0 ? to_light / dist : float3(0.0, 0.0, 0.0);
    float NdotL = max(dot(N, L), 0.0);
    
    //거리에 따른 감쇠(0.0 ~ 1.0)
    //dist가 작을수록(=가까울수록) 1에서 빼기에 fade가 큰 값이 된다.
    //거리가 멀수록 fade는 작아진다.
    float fade = saturate(1.0 - (dist / max(light.range, 1e-4)));
    //fall off가 클수록 더 빠르게 감소한다.
    //fade가 1보다 작은 값이고 falloff가 커지면 더 작아지므로.
    float attenuation = pow(fade, light.falloff);
    
    diffuse = light_color * NdotL * attenuation;
    specular = float3(0.0, 0.0, 0.0);
    
//#if LIGHTING_MODEL_BLINN_PHONG
#if 1
    if (NdotL > 0.0)
    {
        float3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        specular = light_color * pow(NdotH, light.shininess) * attenuation;
    }
#endif
}

void calculate_spot(float3 N, float3 V, LightInfo light, float3 world_position, out float3 diffuse, out float3 specular)
{
    float3 light_color = light.color.rgb * light.intensity;
    
    float3 light_to_point = world_position - light.position;
    float dist = length(light_to_point);
    //normalize
    light_to_point = dist > 0.0 ? light_to_point / dist : float3(0.0, 0.0, 0.0);    
    float3 light_forward = normalize(light.direction);
    
    //forward, light-to-point 둘다 단위 벡터이다.
    //내적하게 되면 cos_theta = |forward||light_to_point|cos(theta)인데 크기가 1이라 내적값이 cos(theta)와 같다.
    float cos_theta = dot(light_forward, light_to_point);
    //반대로 inner, outer 각도의 벡터를 forward에 내적한 결과가 cos(angle)과 같다.
    float inner_cos = cos(light.inner_cone_angle);
    float outer_cos = cos(light.outer_cone_angle);
    
    //forward에 내적된 길이값을 비교하여 cos_theta가 inner와 outer 안팎에 있는지 검사한다.
    //cos_theta < inner라면 내적길이는 cos_theta쪽이 커서 1보다 큰값이 된다. saturate로 잘려서 1.0이다.
    //  즉 inner 안쪽 각도에서는 항상 1.0이다.
    //inner < cos_theta < outer라면 0.0 ~ 1.0으로 줄어든다.
    //outer < cos_theta라면 음수가 되는데 saturate로 0.0이된다.
    //  즉 outer 밖은 항상 0.0이다.
    float spot_factor = saturate((cos_theta - outer_cos) / max(inner_cos - outer_cos, 1e-4));
    
    float NdotL = max(dot(N, -light_to_point), 0.0);
    
    float fade = saturate(1.0 - dist / light.range);
    float attenuation = pow(fade, light.falloff);
    
    diffuse = light_color * NdotL * spot_factor * attenuation;
    specular = float3(0.0, 0.0, 0.0);
    
//#if LIGHTING_MODEL_BLINN_PHONG
#if 1
    if (NdotL > 0.0)
    {
        //카메라 벡터와 라이트 벡터의 중간(하프벡터)과 노멀벡터 내적
        //카메라와 빛의 중앙이 노멀이라면 빛이 눈쪽으로 정반사된다는 뜻
        float3 H = normalize(-light_to_point + V);
        float NdotH = max(dot(N, H), 0.0);
    
        specular = light_color * pow(NdotH, light.shininess) * spot_factor * attenuation;
    }
#endif
}

// Vertex Shader
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 world_position = mul(input.position, Model);
    //어차피 PS에 넘어가면서 보간되어 정규화가 깨진다. Normalize하지 않는다.
    float3 N = mul(input.normal, transpose((float3x3)inv_model));    
    float4 T = float4(mul(input.tangent.xyz, (float3x3) Model), input.tangent.w);
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
    output.tangent = T;
    
	return output;
}

// Pixel Shader
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 output_color = float4(0.0, 0.0, 0.0, 1.0);
    float3 Normal = float3(0.0, 0.0, 0.0);
    
    float3 N = SafeNormalize(input.normal);
    
    // 임시: 정점 Tangent가 없을 때 N에 수직인 아무 T나 만든다
    //float3 axis = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0); // N과 거의 평행하면 다른 축
    //float3 T = normalize(axis - N * dot(axis, N)); // axis에서 N 성분을 빼면 N에 수직
    //float w = 1.0;
    
    float3 T = SafeNormalize(input.tangent.xyz - N * dot(input.tangent.xyz, N));
    
    //직교화 후 크기가 너무 작다면 정점의 노멀을 사용하도록 한다.
    if (dot(T, T) < 1e-8)
    {
        Normal = N;
    }
    else
    {
        //오차가 있어도 정확히 -1 혹은 1이 되도록
        float w = input.tangent.w < 0 ? -1 : 1;
    
        float3 B = cross(N, T) * w;
    
    
    //샘플러랑 uv는 다시 확인.
    //* 2.0 - 1.0으로 색상 -> 벡터 변환.
        float3 SamplingNormal = normal_texture.Sample(default_sampler, input.uv).xyz * 2.0 - 1.0;
        
    //Default NormalMap이라면 0,0,1과 곱하게 되어 버텍스의 노멀값이된다.
    //Default NormalMap은 (0.5, 0.5, 1.0, 1.0)이다. 색상을 벡터 변환하면 0,0,1이 된다.
        float3 MappingNormal = SafeNormalize(mul(SamplingNormal, float3x3(T, B, N)));
        
        Normal = MappingNormal;
    }
    
    
    if (UseWorldNormalView)
    {        
        //픽셀 셰이더에 들어온 보간된 Normal은 길이가 1이 아닐 수 있기에 normalize 해준다.
        //float3 NormalizeNormal = normalize(input.normal);
        //normal을 색으로 변환 
        //-1 ~ 1 -> 0.0 ~ 1.0
        //감마보정을 대비해 2.2승
        output_color.rgb = pow((Normal + 1.0) * 0.5, 2.2);
        
        //아래의 식대로 출력하면 본래 월드 노멀보다 조금 밝은 색이 나온다.
        //이유는 백버퍼에 쓸 때 감마보정이 들어가기 때문
        //2.2승을 하여 출력하면 본래의 노멀 값대로 색상이 적용된다.
        //하지만 노멀 색상을 2.2승하여 출력하게 되면 이 노멀 값을 가지고 계산하게 될 때 틀린 값을 사용하게 된다.
        //지금 당장 계산된 노멀값을 쓰고 있진 않지만 차후 노멀값이 기록된 버퍼를 만들어야 할 때 2.2승을 하면 안된다.
        //output_color.rgb = (MappingNormal + 1.0) * 0.5;
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
    
        //float3 N = normalize(input.normal);
        float3 V = SafeNormalize(view_position - input.world_position);
    
        float3 ambient = ambient_color.rgb * ambient_intensity;
        float3 total_diffuse = float3(0.0, 0.0, 0.0);
        float3 total_specular = float3(0.0, 0.0, 0.0);

        float3 diffuse = float3(0.0, 0.0, 0.0);
        float3 specular = float3(0.0, 0.0, 0.0);
        calculate_diretional(Normal, V, dl_directional, dl_color.rgb, dl_intensity, diffuse, specular);
    
        total_diffuse += diffuse;
        total_specular += specular;
    
        for (int i = 0; i < light_count; ++i)
        {
            LightInfo light = lights[i];        

            if (light.type == 1)
            {
                calculate_point(Normal, V, light, input.world_position, diffuse, specular);
            }
            else if (light.type == 2)
            {
                calculate_spot(Normal, V, light, input.world_position, diffuse, specular);
            }
        
            total_diffuse += diffuse;
            total_specular += specular;
        }
    
        output_color.rgb = output_color.rgb * (ambient + total_diffuse) + total_specular;
#endif
    }
    

    
    return output_color;
}
