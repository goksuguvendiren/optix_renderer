//
// Created by Göksu Güvendiren on 2019-03-01.
//

#include <optixu/optixu_math_namespace.h>
#include "../../optixPathTracer.h"
#include "../../include/light_sources/point.hpp"

struct PerRayData_pathtrace_shadow
{
    bool inShadow;
};

struct PerRayData_pathtrace
{
    float3 result;
    float3 radiance;
    float3 attenuation;
    float3 origin;
    float3 direction;
    unsigned int seed;
    int depth;
    int countEmitted;
    int done;
};

rtDeclareVariable(float3,     diffuse_color, , );
rtDeclareVariable(float3,     geometric_normal, attribute geometric_normal, );
rtDeclareVariable(float3,     shading_normal,   attribute shading_normal, );
rtDeclareVariable(optix::Ray, ray,              rtCurrentRay, );
rtDeclareVariable(float,      t_hit,            rtIntersectionDistance, );

rtDeclareVariable(unsigned int,  frame_number, , );
rtDeclareVariable(unsigned int,  sqrt_num_samples, , );
rtDeclareVariable(unsigned int,  rr_begin_depth, , );
rtDeclareVariable(unsigned int,  pathtrace_ray_type, , );
rtDeclareVariable(unsigned int,  pathtrace_shadow_ray_type, , );
rtDeclareVariable(float,         scene_epsilon, , );
rtDeclareVariable(rtObject,      top_object, , );

rtDeclareVariable(PerRayData_pathtrace, current_prd, rtPayload, );
rtDeclareVariable(PerRayData_pathtrace_shadow, prd_shadow, rtPayload, );

rtBuffer<grpt::point_light>      point_lights;

RT_PROGRAM void closest_hit()
{
    optix::float3 world_geo_normal   = optix::normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
    optix::float3 world_shade_normal = optix::normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
    optix::float3 ffnormal           = optix::faceforward( world_shade_normal, -ray.direction, world_geo_normal );

    float3 Ka = optix::make_float3( 0.3f, 0.3f, 0.3f );
    float3 Kd = optix::make_float3( 0.6f, 0.7f, 0.8f );
    float3 Ks = optix::make_float3( 0.8f, 0.9f, 0.8f );
    float  phong_exp = 88;
    float3 ambient_color = optix::make_float3( 0.31f, 0.33f, 0.28f );

    //ambient
    optix::float3 result = Ka * ambient_color * diffuse_color;
    optix::float3 hit_point = ray.origin + t_hit * ray.direction;

    for (int i = 0; i < point_lights.size(); ++i)
    {
        grpt::point_light light = point_lights[i];

        PerRayData_pathtrace_shadow shadow_payload;2
        shadow_payload.inShadow = false;

        float light_distance = optix::length(light.Position() - hit_point);
        optix::float3 L = optix::normalize(light.Position() - hit_point);
        optix::float3 ray_pos = hit_point + L * scene_epsilon;

        optix::Ray shadow_ray = optix::make_Ray(ray_pos, L, 1, scene_epsilon, light_distance);
        rtTrace(top_object, shadow_ray, shadow_payload);

        if (shadow_payload.inShadow) continue;

        float cos_theta = optix::dot(L, ffnormal);

        if (cos_theta > 0)
        {
            // diffuse term
            result += Ka * cos_theta * light.Emission();

            // specular term
            optix::float3 H = optix::normalize(L - ray.direction);
            float cos_alpha = optix::dot(H, ffnormal);
            if (cos_alpha > 0)
            {
                result += Ks * light.Emission() * pow(cos_alpha, phong_exp);
            }
        }
    }

    current_prd.result = result;
}

rtDeclareVariable(PerRayData_pathtrace_shadow, current_prd_shadow, rtPayload, );

RT_PROGRAM void any_hit()
{
    current_prd_shadow.inShadow = true;
    rtTerminateRay();
}