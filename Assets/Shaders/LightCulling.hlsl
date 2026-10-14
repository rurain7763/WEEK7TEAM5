#define FLT_MAX 3.402823466e+38f
#define FLT_MIN -3.402823466e+38f

#define LEFT_PLANE 0
#define RIGHT_PLANE 1
#define TOP_PLANE 2
#define BOT_PLANE 3

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

struct Plane
{
    float3 normal;
};

cbuffer Constants : register(b0)
{
    row_major float4x4 inv_projection;
    row_major float4x4 view_matrix;
    uint width;
    uint height;
    uint light_count;
    uint tile_x;
};

Texture2D depth_texture : register(t0);
StructuredBuffer<LightInfo> lights : register(t1);

RWStructuredBuffer<int> output : register(u0);

groupshared float max_depths[THREAD_X * THREAD_Y];
groupshared float min_depths[THREAD_X * THREAD_Y];

groupshared Plane planes[4];

groupshared uint light_indices[MAX_LIGHT_INDICES_COUNT];
groupshared uint light_index_count;

[numthreads(THREAD_X, THREAD_Y, 1)]
void mainCS(uint3 id : SV_DispatchThreadID, uint3 group_id : SV_GroupID, uint group_index : SV_GroupIndex)
{
    // NOTE: depth를 기록한 위치는 픽셀의 0.5, 0.5 지점이므로 0.5를 더해준다.
    float screen_x = (((float) id.x + 0.5) / (float) width);
    float screen_y = (((float) id.y + 0.5) / (float) height);
    
    // NOTE: Depth 기록하기
    float max_depth = FLT_MIN;
    float min_depth = FLT_MAX;
    if (id.x < width && id.y < height)
    {
        float ndc_x = screen_x * 2.0 - 1.0;
        float ndc_y = (1.0 - screen_y) * 2.0 - 1.0;
        float ndc_z = depth_texture.Load(id);
        
        float4 view_position = mul(float4(ndc_x, ndc_y, ndc_z, 1.0), inv_projection);
        view_position.xyz /= view_position.w;
        
        max_depth = view_position.z;
        min_depth = view_position.z;
    }
    
    max_depths[group_index] = max_depth;
    min_depths[group_index] = min_depth;
    
    GroupMemoryBarrierWithGroupSync();
    
    // NOTE: Min, Max depth 구하기.
    int stride = (THREAD_X * THREAD_Y) / 2;
    while (stride)
    {
        if (group_index < stride)
        {
            max_depths[group_index] = max(max_depths[group_index], max_depths[group_index + stride]);
            min_depths[group_index] = min(min_depths[group_index], min_depths[group_index + stride]);
        }
        
        GroupMemoryBarrierWithGroupSync();
        
        stride /= 2;
    }
    
    // NOTE: 4개의 사이드 평면을 구한다. (0번째 스레드만)
    if (group_index == 0)
    {
        // NOTE: 자기가 속한 타일의 꼭짓점 계산
        float2 screen_lt = float2(THREAD_X * group_id.x, THREAD_Y * group_id.y);
        float2 screen_rt = float2(screen_lt.x + THREAD_X, screen_lt.y);
        float2 screen_lb = float2(screen_lt.x, screen_lt.y + THREAD_Y);
        float2 screen_rb = float2(screen_lt.x + THREAD_X, screen_lt.y + THREAD_Y);
        screen_lt /= float2(width, height);
        screen_rt /= float2(width, height);
        screen_lb /= float2(width, height);
        screen_rb /= float2(width, height);

        float4 ndc_lt = float4(screen_lt.x * 2.0 - 1.0, (1.0 - screen_lt.y) * 2.0 - 1.0, 0.5, 1.0);
        float4 ndc_rt = float4(screen_rt.x * 2.0 - 1.0, (1.0 - screen_rt.y) * 2.0 - 1.0, 0.5, 1.0);
        float4 ndc_lb = float4(screen_lb.x * 2.0 - 1.0, (1.0 - screen_lb.y) * 2.0 - 1.0, 0.5, 1.0);
        float4 ndc_rb = float4(screen_rb.x * 2.0 - 1.0, (1.0 - screen_rb.y) * 2.0 - 1.0, 0.5, 1.0);

        float4 view_lt = mul(ndc_lt, inv_projection);
        view_lt.xyz /= view_lt.w;
    
        float4 view_rt = mul(ndc_rt, inv_projection);
        view_rt.xyz /= view_rt.w;
    
        float4 view_lb = mul(ndc_lb, inv_projection);
        view_lb.xyz /= view_lb.w;
    
        float4 view_rb = mul(ndc_rb, inv_projection);
        view_rb.xyz /= view_rb.w;
    
        planes[LEFT_PLANE].normal = normalize(cross(view_lt.xyz, view_lb.xyz));
        planes[RIGHT_PLANE].normal = normalize(cross(view_rb.xyz, view_rt.xyz));
        planes[TOP_PLANE].normal = normalize(cross(view_rt.xyz, view_lt.xyz));
        planes[BOT_PLANE].normal = normalize(cross(view_lb.xyz, view_rb.xyz));
        
        light_index_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();
    
    // NOTE: 조명 검사를 수행한다.
    uint thread_count = THREAD_X * THREAD_Y;
    
    max_depth = max_depths[0];
    min_depth = min_depths[0];
    for (int i = group_index; i < light_count; i += thread_count)
    {
        LightInfo light = lights[i];
        
        if (light.type == 1)
        {
            float4 view_position = mul(float4(light.position, 1.0), view_matrix);
            float depth = view_position.z;
            
            if (depth - light.range > max_depth || depth + light.range < min_depth)
            {
                continue;
            }
            
            bool skip = false;
            for (int j = 0; j < 4; j++)
            {
                if (dot(planes[j].normal, view_position.xyz) < -light.range)
                {
                    skip = true;
                    break;
                }
            }
            
            if (skip)
            {
                continue;
            }
        }
        else if (light.type == 2)
        {
            // NOTE: 일단 원뿔을 감싸는 원뿔 형태로 구현.
            float radius = light.range * tan(light.outer_cone_angle);
            float half_height = light.range * 0.5;
            float4 volume_sphere_view_position = mul(float4(light.position + half_height * light.direction, 1.0), view_matrix);
            float volume_sphere_radius = sqrt(half_height * half_height + radius * radius);
            float depth = volume_sphere_view_position.z;

            if (depth - volume_sphere_radius > max_depth || depth + volume_sphere_radius < min_depth)
            {
                continue;
            }
            
            bool skip = false;
            for (int j = 0; j < 4; j++)
            {
                if (dot(planes[j].normal, volume_sphere_view_position.xyz) < -volume_sphere_radius)
                {
                    skip = true;
                    break;
                }
            }
            
            if (skip)
            {
                continue;
            }
        }
            
        uint insert_pos;
        InterlockedAdd(light_index_count, 1, insert_pos);
        
        // TODO: 조명 누락 시 어떻게 처리할까? 지금은 단순히 소실된다.
        if (insert_pos < MAX_LIGHT_INDICES_COUNT)
        {
            light_indices[insert_pos] = i;
        }
    }
    
    GroupMemoryBarrierWithGroupSync();

    // NOTE: 계산된 라이트를 기록한다.
    uint tile_index = group_id.y * tile_x + group_id.x;
    uint write_pos = tile_index * MAX_LIGHT_INDICES_COUNT;
    
    uint clamped = min(light_index_count, MAX_LIGHT_INDICES_COUNT);
    if (group_index < clamped)
    {
        for (int i = group_index; i < clamped; i += thread_count)
        {
            output[write_pos + i] = light_indices[i];
        }
    }
    
    if (clamped < MAX_LIGHT_INDICES_COUNT && group_index == 0)
    {
        output[write_pos + light_index_count] = -1;
    }
    
    GroupMemoryBarrierWithGroupSync();
}
