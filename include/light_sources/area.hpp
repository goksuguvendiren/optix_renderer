//
// Created by Göksu Güvendiren on 2019-03-02.
//

#pragma once

#include <optixu/optixu_math_namespace.h>

namespace grpt
{
    class area_light
    {
    public:
        area_light(){}
        area_light(const optix::float3& c, const optix::float3& a, const optix::float3& b, const optix::float3& ems) : corner(c), v1(a), v2(b), emission(ems)
        {
            normal = optix::normalize(optix::cross(v1, v2));
        }

        optix::float3 corner;
        optix::float3 v1, v2;
        optix::float3 normal;
        optix::float3 emission;
    };
}