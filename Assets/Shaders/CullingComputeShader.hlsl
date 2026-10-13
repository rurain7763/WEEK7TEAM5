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


cbuffer CullingConstants : register(b0)
{
    float viewport_width;
    float viewport_length;
    float2 tile_size;
    float tile_amount;
    row_major matrix view;
    float camera_near;
    float camera_far;
    int light_count;
}

RWStructuredBuffer<float> TileLightnum : register(u0);
RWStructuredBuffer<float> TileLightindex : register(u1);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint2 tile = id.xy; 

    if (tile.x >= tile_size.x || tile.y >= tile_size.y) return;

    uint tileIndex = tile.y * tile_size.x + tile.x;
}