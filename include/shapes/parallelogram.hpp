//
// Created by Göksu Güvendiren on 2019-03-06.
//

#pragma once

namespace grpt
{
    class parallelogram
    {
    public:
        optix::float3 anchor;
        optix::float3 off1;
        optix::float3 off2;

        std::string material;
        optix::float3 color;

        parallelogram(const optix::float3& anc, const optix::float3& o1, const optix::float3& o2, const std::string& mat, const optix::float3& col) : anchor(anc), off1(o1), off2(o2), material(mat), color(col)
        {}
    };
}