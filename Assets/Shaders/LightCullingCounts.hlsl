cbuffer CountConstants : register(b0)
{
    float viewport_width;
    float viewport_height;
    uint total_light_count;
    uint padding;
}

struct TileInfo
{
    float TileLightnum;
    float TileLightindex[200];
};

StructuredBuffer<TileInfo> TileInfomation : register(t0);

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

uint LightCountDigits(uint value)
{
    return value >= 100u ? 3u : (value >= 10u ? 2u : 1u);
}

uint LightCountDigit(uint value, uint index, uint digits)
{
    uint remaining = digits - index;
    uint divisor = remaining == 3u ? 100u : (remaining == 2u ? 10u : 1u);
    return (value / divisor) % 10u;
}

uint LightCountGlyph(uint digit)
{
    const uint masks[10] = { 31599u, 29850u, 29671u, 31207u, 18925u,
                            31183u, 31695u, 18727u, 31727u, 31215u };
    return masks[min(digit, 9u)];
}

float4 DrawLightCountOverlay(float2 screenPosition, uint listed, uint total)
{
    uint2 viewportPixels = uint2(viewport_width, viewport_height);
    if (any(viewportPixels == 0u))
        return float4(0.0, 0.0, 0.0, 0.0);

    uint2 pixel = uint2(floor(screenPosition));
    uint2 tile = min(((pixel + 1u) * 32u - 1u) / viewportPixels, uint2(31u, 31u));
    float2 tileStart = floor(float2(tile) * float2(viewportPixels) / 32.0);
    float2 tileEnd = floor(float2(tile + 1u) * float2(viewportPixels) / 32.0);
    float2 tilePixels = tileEnd - tileStart;
    float2 localPixel = screenPosition - tileStart;

    if (localPixel.x < 1.0 || localPixel.y < 1.0)
        return float4(0.1, 0.8, 1.0, 0.75);

    uint shownListed = min(listed, 999u);
    uint shownTotal = min(total, 999u);
    uint listedDigits = LightCountDigits(shownListed);
    uint totalDigits = LightCountDigits(shownTotal);
    uint characters = listedDigits + 1u + totalDigits;
    float textWidth = float(characters * 4u - 1u);
    float scale = min(2.0, min((tilePixels.x - 2.0) / textWidth, (tilePixels.y - 2.0) / 5.0));
    if (scale < 0.5)
        return float4(0.0, 0.0, 0.0, 0.0);

    float2 origin = (tilePixels - float2(textWidth, 5.0) * scale) * 0.5;
    float2 glyphPosition = (localPixel - origin) / scale;
    if (glyphPosition.x < -0.5 || glyphPosition.x >= textWidth + 0.5 ||
        glyphPosition.y < -0.5 || glyphPosition.y >= 5.5)
        return float4(0.0, 0.0, 0.0, 0.0);

    float4 background = float4(0.01, 0.02, 0.03, 0.85);
    if (glyphPosition.x < 0.0 || glyphPosition.x >= textWidth ||
        glyphPosition.y < 0.0 || glyphPosition.y >= 5.0)
        return background;

    uint character = uint(floor(glyphPosition.x / 4.0));
    uint x = uint(floor(glyphPosition.x)) % 4u;
    uint y = uint(floor(glyphPosition.y));
    if (x >= 3u)
        return background;

    bool ink;
    if (character == listedDigits)
    {
        ink = x == 2u - y / 2u;
    }
    else
    {
        uint digit;
        if (character < listedDigits)
            digit = LightCountDigit(shownListed, character, listedDigits);
        else
            digit = LightCountDigit(shownTotal, character - listedDigits - 1u, totalDigits);
        ink = (LightCountGlyph(digit) & (1u << (y * 3u + x))) != 0u;
    }

    float3 textColor = listed < total ? float3(0.3, 1.0, 0.45) : float3(1.0, 1.0, 1.0);
    return ink ? float4(textColor, 1.0) : background;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    uint2 viewportPixels = uint2(viewport_width, viewport_height);
    if (any(viewportPixels == 0u))
        return float4(0.0, 0.0, 0.0, 0.0);

    uint2 pixel = uint2(floor(input.position.xy));
    uint2 tile = min(((pixel + 1u) * 32u - 1u) / viewportPixels, uint2(31u, 31u));
    uint tileIndex = tile.y * 32u + tile.x;
    uint listedLights = uint(TileInfomation[tileIndex].TileLightnum);
    return DrawLightCountOverlay(input.position.xy, listedLights, total_light_count);
}
