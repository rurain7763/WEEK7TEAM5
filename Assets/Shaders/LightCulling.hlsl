#define FLT_MAX 3.402823466e+38f
#define FLT_MIN -3.402823466e+38f

#define PI 3.14159265359

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

// NOTE: 원점을 지나지 않는 평면도 표현할 수 있도록 d를 둔다. (직교 투영의 타일 측면은 원점을 지나지 않는다)
// 점 p까지의 부호 있는 거리 = dot(normal, p) + d, 양수가 타일 안쪽.
struct Plane
{
    float3 normal;
    float d;
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

// NOTE: 타일의 최소~최대 깊이 구간을 감싸는 뷰 공간 AABB
groupshared float3 tile_aabb_min;
groupshared float3 tile_aabb_max;

groupshared uint light_indices[MAX_LIGHT_INDICES_COUNT];
groupshared uint light_index_count;

// NOTE: NDC 좌표를 뷰 공간 위치로 되돌린다.
float3 UnprojectToView(float2 ndc_xy, float ndc_z)
{
    float4 view_position = mul(float4(ndc_xy, ndc_z, 1.0), inv_projection);
    return view_position.xyz / view_position.w;
}

// NOTE: 세 점을 지나는 평면을 만들고, inside_point가 양수 쪽이 되도록 방향을 맞춘다.
Plane MakePlane(float3 a, float3 b, float3 c, float3 inside_point)
{
    Plane plane;
    plane.normal = normalize(cross(b - a, c - a));
    plane.d = -dot(plane.normal, a);

    if (dot(plane.normal, inside_point) + plane.d < 0.0)
    {
        plane.normal = -plane.normal;
        plane.d = -plane.d;
    }

    return plane;
}

// NOTE: 구가 타일의 측면 평면 4개 안쪽에 걸쳐 있고, 타일의 깊이 구간 AABB와도 닿는지 검사한다.
// 평면 검사만으로는 타일 대각선 방향의 구와 타일 깊이 구간 밖의 구를 걸러내지 못하므로 AABB 검사를 함께 한다.
bool SphereIntersectsTile(float3 center, float radius)
{
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (dot(planes[i].normal, center) + planes[i].d < -radius)
        {
            return false;
        }
    }

    float3 closest = clamp(center, tile_aabb_min, tile_aabb_max);
    float3 offset = center - closest;
    return dot(offset, offset) <= radius * radius;
}

// NOTE: 스폿 라이트(원뿔 + 거리 범위)를 감싸는 가장 작은 구 (Bart Wronski, "Cull that cone")
// outer 각이 90도에 가까워져도 반지름이 range를 넘지 않는다.
void GetSpotBoundingSphere(LightInfo light, out float3 center, out float radius)
{
    float3 direction = normalize(light.direction);
    float angle = light.outer_cone_angle;

    if (angle > PI * 0.25)
    {
        radius = sin(angle) * light.range;
        center = light.position + direction * (cos(angle) * light.range);
    }
    else
    {
        radius = light.range / (2.0 * cos(angle));
        center = light.position + direction * radius;
    }
}

[numthreads(THREAD_X, THREAD_Y, 1)]
void mainCS(uint3 id : SV_DispatchThreadID, uint3 group_id : SV_GroupID, uint group_index : SV_GroupIndex)
{
    // NOTE: Depth 기록하기
    // 타일 꼭짓점을 이 깊이로 역투영해 AABB를 만들기 때문에, 뷰 공간 z가 아니라 NDC 깊이 그대로 기록한다.
    // (원근/직교/혼합 투영 모두 NDC 깊이는 앞뒤 순서를 유지한다)
    float max_depth = FLT_MIN;
    float min_depth = FLT_MAX;
    if (id.x < width && id.y < height)
    {
        float ndc_z = depth_texture.Load(id);

        max_depth = ndc_z;
        min_depth = ndc_z;
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
    
    // NOTE: 타일의 측면 평면 4개와 깊이 구간 AABB를 구한다. (0번째 스레드만)
    if (group_index == 0)
    {
        // NOTE: 자기가 속한 타일의 픽셀 범위. 화면 오른쪽/아래 끝 타일은 화면 밖 부분을 잘라낸다.
        float2 screen_min = float2(THREAD_X * group_id.x, THREAD_Y * group_id.y);
        float2 screen_max = min(screen_min + float2(THREAD_X, THREAD_Y), float2(width, height));
        screen_min /= float2(width, height);
        screen_max /= float2(width, height);

        float ndc_left = screen_min.x * 2.0 - 1.0;
        float ndc_right = screen_max.x * 2.0 - 1.0;
        float ndc_top = 1.0 - screen_min.y * 2.0;
        float ndc_bottom = 1.0 - screen_max.y * 2.0;

        float2 corners[4] =
        {
            float2(ndc_left, ndc_top),      // lt
            float2(ndc_right, ndc_top),     // rt
            float2(ndc_left, ndc_bottom),   // lb
            float2(ndc_right, ndc_bottom),  // rb
        };

        // NOTE: 측면 평면은 near(0) ~ far(1) 전체 구간의 꼭짓점 3개로 만든다.
        // 원점을 지나는 평면으로 만들면 원근 투영에서만 맞고 직교 투영에서는 틀린다.
        float3 near_points[4];
        float3 far_points[4];
        for (int c = 0; c < 4; c++)
        {
            near_points[c] = UnprojectToView(corners[c], 0.0);
            far_points[c] = UnprojectToView(corners[c], 1.0);
        }

        float3 inside_point = UnprojectToView(float2((ndc_left + ndc_right) * 0.5, (ndc_top + ndc_bottom) * 0.5), 0.5);

        planes[LEFT_PLANE] = MakePlane(near_points[0], near_points[2], far_points[0], inside_point);
        planes[RIGHT_PLANE] = MakePlane(near_points[1], near_points[3], far_points[1], inside_point);
        planes[TOP_PLANE] = MakePlane(near_points[0], near_points[1], far_points[0], inside_point);
        planes[BOT_PLANE] = MakePlane(near_points[2], near_points[3], far_points[2], inside_point);

        // NOTE: 실제 픽셀이 있는 깊이 구간(min ~ max)만 감싸는 AABB.
        // 배경만 있는 타일은 min == max == 1(far)이라 far 평면에 붙은 얇은 상자가 된다.
        float tile_min_depth = min_depths[0];
        float tile_max_depth = max_depths[0];

        float3 aabb_min = float3(FLT_MAX, FLT_MAX, FLT_MAX);
        float3 aabb_max = float3(FLT_MIN, FLT_MIN, FLT_MIN);
        for (int k = 0; k < 4; k++)
        {
            float3 p0 = UnprojectToView(corners[k], tile_min_depth);
            float3 p1 = UnprojectToView(corners[k], tile_max_depth);
            aabb_min = min(aabb_min, min(p0, p1));
            aabb_max = max(aabb_max, max(p0, p1));
        }

        tile_aabb_min = aabb_min;
        tile_aabb_max = aabb_max;

        light_index_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    // NOTE: 조명 검사를 수행한다.
    uint thread_count = THREAD_X * THREAD_Y;

    for (int i = group_index; i < light_count; i += thread_count)
    {
        LightInfo light = lights[i];

        if (light.type == 1)
        {
            float3 view_center = mul(float4(light.position, 1.0), view_matrix).xyz;
            if (!SphereIntersectsTile(view_center, light.range))
            {
                continue;
            }
        }
        else if (light.type == 2)
        {
            // NOTE: 원뿔을 감싸는 구로 근사한다.
            float3 world_center;
            float sphere_radius;
            GetSpotBoundingSphere(light, world_center, sphere_radius);

            float3 view_center = mul(float4(world_center, 1.0), view_matrix).xyz;
            if (!SphereIntersectsTile(view_center, sphere_radius))
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
