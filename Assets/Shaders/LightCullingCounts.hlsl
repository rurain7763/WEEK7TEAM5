cbuffer CountConstants : register(b0)
{
    uint viewport_width;
    uint viewport_height;
    uint total_light_count;
    uint tile_count_x;
    uint tile_width;
    uint tile_height;
    uint max_light_count;
    uint padding;
}

StructuredBuffer<uint> tile_light_counts : register(t0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
};

PS_INPUT mainVS(uint vertex_id : SV_VertexID)
{
    PS_INPUT output;
    float2 uv = float2((vertex_id << 1u) & 2u, vertex_id & 2u);
    output.position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return output;
}

float4 DrawLightCountOverlay(float2 screenPosition, uint2 tile, uint listed)
{
    uint2 tileSize = uint2(tile_width, tile_height);
    float2 localPixel = screenPosition - float2(tile * tileSize);
    if (localPixel.x < 1.0 || localPixel.y < 1.0)
        return float4(0.35, 0.40, 0.43, 0.12);

    if (listed == 0u)
        return float4(0.0, 0.0, 0.0, 0.0);

    // Fixed scale: 1 blue, 6 green, 11 yellow, 16 or more red.
    float level = saturate((float(listed) - 1.0) / 15.0) * 3.0;
    float3 blue = float3(0.15, 0.40, 0.90);
    float3 green = float3(0.20, 0.75, 0.35);
    float3 yellow = float3(0.95, 0.80, 0.15);
    float3 red = float3(0.90, 0.20, 0.15);
    float3 color;
    if (level < 1.0)
        color = lerp(blue, green, level);
    else if (level < 2.0)
        color = lerp(green, yellow, level - 1.0);
    else
        color = lerp(yellow, red, level - 2.0);
    return float4(color, 0.35);
}
float4 mainPS(PS_INPUT input) : SV_TARGET
{
    uint2 viewportPixels = uint2(viewport_width, viewport_height);
    uint2 tileSize = uint2(tile_width, tile_height);
    if (any(viewportPixels == 0u) || any(tileSize == 0u))
        return float4(0.0, 0.0, 0.0, 0.0);

    uint2 pixel = uint2(floor(input.position.xy));
    if (any(pixel >= viewportPixels))
        return float4(0.0, 0.0, 0.0, 0.0);

    uint2 tile = pixel / tileSize;
    uint tileIndex = tile.y * tile_count_x + tile.x;
    uint listedLights = tile_light_counts[tileIndex];
    return DrawLightCountOverlay(input.position.xy, tile, listedLights);
}
