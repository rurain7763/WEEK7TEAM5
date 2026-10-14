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

StructuredBuffer<int> light_culling_indices : register(t0);
RWStructuredBuffer<uint> tile_light_counts : register(u0);

[numthreads(8, 8, 1)]
void mainCS(uint3 id : SV_DispatchThreadID)
{
    uint tile_count_y = (viewport_height + tile_height - 1u) / tile_height;
    if (id.x >= tile_count_x || id.y >= tile_count_y)
        return;

    uint tile_index = id.y * tile_count_x + id.x;
    uint start_index = tile_index * max_light_count;
    uint limit = min(total_light_count, max_light_count);
    uint count = 0u;
    [loop]
    for (uint i = 0u; i < limit; ++i)
    {
        if (light_culling_indices[start_index + i] == -1)
            break;
        ++count;
    }
    tile_light_counts[tile_index] = count;
}
