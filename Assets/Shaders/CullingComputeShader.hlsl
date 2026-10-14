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

struct FCellBounds
{
    float3 ViewAabbMin;
    float3 ViewAabbMax;
    float3 ViewCenter;
    float3 ViewExtent;
};

struct FTile
{
    float2 position;
    float2 pixel_position;
    float2 start_position;
    float2 end_position;
    uint tileindex;
    FCellBounds AABB;
};

cbuffer CullingConstants : register(b0)
{
    float viewport_width;
    float viewport_height;
    float2 tile_size;
    float tile_amount;
    float3 pad1;
    row_major matrix view;
    row_major matrix inv_projection;
    int light_count;
    float3 pad2;
}

struct TileInfo
{
   float TileLightnum;
   float TileLightindex[200];
};

RWStructuredBuffer<TileInfo> TileInfomation: register(u0);

StructuredBuffer<LightInfo> lights : register(t0);

float3 PixeltoView(float2 pixelposition,bool isDepthNear)
{
    float ndcX = 2 * pixelposition.x / viewport_width - 1;
    float ndcY = 1 - 2 * pixelposition.y / viewport_height;
    float depth;
    if (isDepthNear)
    {
        depth = 0;
    }
    else depth = 1;
    float4 projectedPoint = float4(ndcX, ndcY, depth, 1.0);
    float4 viewPointH = mul(projectedPoint, inv_projection);
    float3 viewPoint = viewPointH.xyz / viewPointH.w;
    return viewPoint;
}

void ComputeCellBounds(inout FTile tile)
{
    float3 tilefront1 = PixeltoView(tile.start_position, true);
    float3 tilefront2 = PixeltoView(float2(tile.end_position.x, tile.start_position.y), true);
    float3 tilefront3 = PixeltoView(tile.end_position, true);
    float3 tilefront4 = PixeltoView(float2(tile.start_position.x, tile.end_position.y), true);
    float3 tileback1 = PixeltoView(tile.start_position, false);
    float3 tileback2 = PixeltoView(float2(tile.end_position.x, tile.start_position.y), false);
    float3 tileback3 = PixeltoView(tile.end_position, false);
    float3 tileback4 = PixeltoView(float2(tile.start_position.x, tile.end_position.y), false);

    float3 currentMin = tilefront1;
    float3 currentMax = tilefront1;
    
    float3 maxminarray[8] = { tilefront1, tilefront2, tilefront3, tilefront4, tileback1, tileback2, tileback3, tileback4 };
    for (int i = 1; i < 8; i++)
    {
        currentMin = float3(min(currentMin.x, maxminarray[i].x), min(currentMin.y, maxminarray[i].y), min(currentMin.z, maxminarray[i].z));
        currentMax = float3(max(currentMax.x, maxminarray[i].x), max(currentMax.y, maxminarray[i].y), max(currentMax.z, maxminarray[i].z));
    }

    tile.AABB.ViewAabbMin = currentMin;
    tile.AABB.ViewAabbMax = currentMax;
    tile.AABB.ViewCenter = (currentMin + currentMax) / 2;
    tile.AABB.ViewExtent = (currentMax - currentMin) / 2;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{ 
    FTile tile = (FTile) 0;
    
    tile.position = id.xy;
    tile.start_position = float2(floor(id.x * viewport_width / tile_size.x),floor(id.y * viewport_height / tile_size.y));
    tile.end_position = float2(floor((id.x + 1) * viewport_width / tile_size.x),floor((id.y + 1) * viewport_height / tile_size.y));
    if (tile.position.x >= tile_size.x || tile.position.y >= tile_size.y)
        return;
    tile.tileindex = uint(tile.position.y * tile_size.x) + uint(tile.position.x);
    ComputeCellBounds(tile);
    uint count = 0;
    for (int i = 0; i < light_count; ++i)
    {
        LightInfo light = lights[i];
        float3 Lightposition = mul(float4(light.position,1.0f),view).xyz;
        if (light.type == 0)
        {
            TileInfomation[tile.tileindex].TileLightindex[count] = i;
            count++;
        }
        if (light.type == 1)
        {
            if ((tile.AABB.ViewAabbMin.x - light.range < Lightposition.x && tile.AABB.ViewAabbMax.x + light.range > Lightposition.x)
                && (tile.AABB.ViewAabbMin.y - light.range < Lightposition.y && tile.AABB.ViewAabbMax.y + light.range > Lightposition.y)
                && (tile.AABB.ViewAabbMin.z - light.range < Lightposition.z && tile.AABB.ViewAabbMax.z + light.range > Lightposition.z))
            {
                TileInfomation[tile.tileindex].TileLightindex[count] = i;
                count++;
            }          
        }
        if (light.type == 2)
        {
            if ((tile.AABB.ViewAabbMin.x - light.range < Lightposition.x && tile.AABB.ViewAabbMax.x + light.range > Lightposition.x)
                && (tile.AABB.ViewAabbMin.y - light.range < Lightposition.y && tile.AABB.ViewAabbMax.y + light.range > Lightposition.y)
                && (tile.AABB.ViewAabbMin.z - light.range < Lightposition.z && tile.AABB.ViewAabbMax.z + light.range > Lightposition.z))
            {
                TileInfomation[tile.tileindex].TileLightindex[count] = i;
                count++;
            }
        }
    }
    TileInfomation[tile.tileindex].TileLightnum = count;
}

