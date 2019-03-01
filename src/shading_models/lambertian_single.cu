//
// Created by Goksu Guvendiren on 02.20.2019
//

#include <optixu/optixu_math_namespace.h>
#include "../../optixPathTracer.h"
#include "../../include/point_light.hpp"
#include "random.h"

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

//-----------------------------------------------------------------------------
//
//  Lambertian surface closest-hit
//
//-----------------------------------------------------------------------------

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

rtBuffer<ParallelogramLight>     lights;
rtBuffer<grpt::point_light>      point_lights;

RT_PROGRAM void diffuse()
{
    float3 world_shading_normal   = optix::normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
    float3 world_geometric_normal = optix::normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
    float3 ffnormal = optix::faceforward( world_shading_normal, -ray.direction, world_geometric_normal );

    float3 hitpoint = ray.origin + t_hit * ray.direction;

    //
    // Generate a reflection ray.  This will be traced back in ray-gen.
    //
    current_prd.origin = hitpoint;

    float z1=rnd(current_prd.seed);
    float z2=rnd(current_prd.seed);
    float3 p;
    optix::cosine_sample_hemisphere(z1, z2, p);
    optix::Onb onb( ffnormal );
    onb.inverse_transform( p );
    current_prd.direction = p;

    current_prd.attenuation = current_prd.attenuation * diffuse_color;
    current_prd.countEmitted = false;

    //
    // Next event estimation (compute direct lighting).
    //
    unsigned int num_lights = lights.size();
    float3 result = make_float3(0.0f);

    num_lights = point_lights.size();
    for(int i = 0; i < num_lights; ++i)
    {
        grpt::point_light light = point_lights[i];
        const float3 light_pos = light.Position();

        // Calculate properties of light sample (for area based pdf)
        const float  Ldist    = optix::length(light_pos - hitpoint);
        const float3 L        = optix::normalize(light_pos - hitpoint);
        const float  costheta = optix::dot( ffnormal, L );

        // cast shadow ray
        if ( costheta > 0.0f)
        {
//            result = optix::make_float3(0.3);
            PerRayData_pathtrace_shadow shadow_prd;
            shadow_prd.inShadow = false;
            // Note: bias both ends of the shadow ray, in case the light is also present as geometry in the scene.
            optix::Ray shadow_ray = optix::make_Ray( hitpoint, L, pathtrace_shadow_ray_type, scene_epsilon, Ldist - scene_epsilon );
            rtTrace(top_object, shadow_ray, shadow_prd);

            if(!shadow_prd.inShadow)
            {
//                const float A = 1;//optix::length(optix::cross(light.v1, light.v2));
                // convert area based pdf to solid angle
                optix::float3 color = optix::make_float3(0.3);//light.Emission();
                result += color;
            }
//            else
//                result += optix::make_float3(0.8);
        }
    }

    current_prd.radiance = result;
}

//-----------------------------------------------------------------------------
//
//  Shadow any-hit
//
//-----------------------------------------------------------------------------

rtDeclareVariable(PerRayData_pathtrace_shadow, current_prd_shadow, rtPayload, );

RT_PROGRAM void shadow()
{
    current_prd_shadow.inShadow = true;
    rtTerminateRay();
}

rtDeclareVariable(float3,        emission_color, , );

RT_PROGRAM void diffuseEmitter()
{
    current_prd.radiance = current_prd.countEmitted ? emission_color : make_float3(0.f);
    current_prd.done = true;
}

