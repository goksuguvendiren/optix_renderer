//
// Created by Göksu Güvendiren on 2019-02-24.
//

#pragma once

#include <optixu_math_namespace.h>

namespace grpt
{
    class point_light
    {
    public:
        point_light(const optix::float3& pos, const optix::float3& intens) : position(pos), emission(intens) {}

        optix::float3 position;
        optix::float3 emission;

        optix::float3 Position()  const { return position; }
        optix::float3 Emission() const { return emission; }
    };
}