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
    float2 tileindex;
    FCellBounds AABB;
};

cbuffer CullingConstants : register(b0)
{
    float viewport_width;
    float viewport_height;
    float2 tile_size;
    float tile_amount;
    row_major matrix view;
    row_major matrix inv_projection;
    float camera_near;
    float camera_far;
    int light_count;
}

RWStructuredBuffer<float> TileLightnum : register(u0);
RWStructuredBuffer<float> TileLightindex : register(u1);

void ComputeCellBounds(FTile tile)
{
    float3 tilefront1 = mul(float3(tile.start_position, camera_near), (float3x3)inv_projection);
    float3 tilefront2 = mul(float3(tile.end_position.x, tile.start_position.y, camera_near), (float3x3) inv_projection);
    float3 tilefront3 = mul(float3(tile.end_position, camera_near), (float3x3) inv_projection);
    float3 tilefront4 = mul(float3(tile.start_position.x, tile.end_position.y, camera_near), (float3x3) inv_projection);
    float3 tileback1 = mul(float3(tile.start_position, camera_far), (float3x3) inv_projection);
    float3 tileback2 = mul(float3(tile.end_position.x, tile.start_position.y, camera_far), (float3x3) inv_projection);
    float3 tileback3 = mul(float3(tile.end_position, camera_far), (float3x3) inv_projection));
    float3 tileback4 = mul(float3(tile.start_position.x, tile.end_position.y, camera_far), (float3x3) inv_projection);
    
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
    tile.AABB.ViewCenter = currentMin + currentMax / 2;
    tile.AABB.ViewExtent = currentMax + currentMin / 2;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    FTile tile;
    tile.position = id.xy;
    uint2 tile_pixel_size = (viewport_width / tile_size.x, viewport_height / tile_size.y);
    tile.start_position = float2(id.x * tile_pixel_size.x, id.y * tile_pixel_size.y);
    tile.end_position = float2(tile.start_position.x + tile_pixel_size.x, tile.start_position.y + tile_pixel_size.y);
    if (tile.position.x >= tile_size.x || tile.position.y >= tile_size.y) return;
    tile.tileindex = tile.position.y * tile_size.x + tile.position.x;
    
    tile.AABB = ComputeCellBounds(tile);

}

