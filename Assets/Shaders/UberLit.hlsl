
cbuffer ModelConstants : register(b0)
{
    row_major matrix Model;
    float4 Color;
    float2 UVOffset;
    int UseVertexColor;
    int HasTexture;
    
    int LightCount;
    int3 Padding;
    
    row_major matrix ModelInversedTranspose;
};

cbuffer ViewConstants : register(b1)
{
    row_major matrix ViewProjection;
};

struct FLightInfo
{
    float3 Position;
    uint Type;
    float4 Color;
    
    float3 Direction;
    float Intensity;
    
    float Range;
    float FallOff;
    float InnerConeCos;
    float OuterConeCos;
};

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
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

// Todo: Maybe use define
static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;
static const uint LIGHT_TYPE_AMBIENT = 3;

Texture2D MainTexture : register(t0);
StructuredBuffer<FLightInfo> LightInfos : register(t1);

SamplerState default_sampler : register(s0);

float CalculateSpotLightAttenuation(float coneCos, float innerConeCos, float outerConeCos)
{
    if (innerConeCos <= outerConeCos)
    {
        return (coneCos >= outerConeCos) ? 1.0 : 0.0;
    }

    // innerConeCos is bigger than outerConeCos, since inner angle is smaller than outer angle
    return smoothstep(outerConeCos, innerConeCos, coneCos);
}

float CalculatePointLightAttenuation(float distanceSquared, float radius, float falloffExponent)
{
    if (radius <= 0.0)
    {
        return 0.0;
    }
    
    float normalizedDistanceSquared = distanceSquared / (radius * radius);
    float baseAttenuation = saturate(1.0 - normalizedDistanceSquared);

    return pow(baseAttenuation, max(falloffExponent, 0.001));
}

float3 GetToLightDirection(FLightInfo lightInfo, float3 vertexWorldPosition)
{
    float3 toLight = lightInfo.Position - vertexWorldPosition;
    float distanceSquared = dot(toLight, toLight);

    return toLight * rsqrt(max(distanceSquared, 1e-8));
}

float3 CalculateLighting(float3 vertexWorldPosition, float3 vertexWorldNormal)
{
    float3 lighting = float3(0.0, 0.0, 0.0);

    [loop]
    for (int i = 0; i < LightCount; ++i)
    {
        FLightInfo lightInfo = LightInfos[i];

        float3 lightColor = lightInfo.Color.rgb * lightInfo.Intensity;
        
        // Todo: Code duplicate, fix later
        float3 toLight = lightInfo.Position - vertexWorldPosition;
        float distanceSquared = dot(toLight, toLight);

        float distanceAttenuation = CalculatePointLightAttenuation(distanceSquared, lightInfo.Range, lightInfo.FallOff);
        float3 toLightDirection = toLight * rsqrt(max(distanceSquared, 1e-8));

        float diffuseFactor = saturate(dot(vertexWorldNormal, toLightDirection));
        
        if (lightInfo.Type == LIGHT_TYPE_AMBIENT)
        {
            lighting += lightColor;
        }
        else if (lightInfo.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            toLightDirection = -normalize(lightInfo.Direction);
            diffuseFactor = saturate(dot(vertexWorldNormal, toLightDirection));
            
            lighting += (lightColor * diffuseFactor);
        }
        else if (lightInfo.Type == LIGHT_TYPE_POINT)
        {
            lighting += (lightColor * diffuseFactor * distanceAttenuation);
        }
        else if (lightInfo.Type == LIGHT_TYPE_SPOT)
        {
            float coneCos = dot(lightInfo.Direction, -toLightDirection);
            float coneAttenuation = CalculateSpotLightAttenuation(coneCos, lightInfo.InnerConeCos, lightInfo.OuterConeCos);

            lighting += (lightColor * diffuseFactor * distanceAttenuation * coneAttenuation);
        }
    }

    return lighting;
}

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    float4 worldPosition = mul(float4(input.position.xyz, 1.0), Model);
    output.position = mul(worldPosition, ViewProjection);

    if (UseVertexColor != 0)
    {
        output.color = input.color;
    }
    else
    {
        output.color = Color;
    }

    float3 vertexWorldNormal = normalize(mul(float4(input.normal, 0.0), ModelInversedTranspose).xyz);
    float3 lighting = CalculateLighting(worldPosition.xyz, vertexWorldNormal);
    
    output.color.rgb *= lighting;
    output.uv = input.uv + UVOffset;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 finalColor = input.color;
    if (HasTexture != 0)
    {
        finalColor *= MainTexture.Sample(default_sampler, input.uv);
    }

    return finalColor;
}
