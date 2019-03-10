//
// Created by Göksu Güvendiren on 2019-03-09.
//

#pragma once

#include <materials/base_material.hpp>

namespace grpt
{
    class blinn_phong : public base_material
    {
        optix::float3 diffuse_color;
        optix::float3 specular_color;
        float         exponent;

    public:
        blinn_phong(optix::Context context, const std::string& material_source_path, const std::string& closest_hit,
                const std::string& any_hit, const optix::float3& dc, const optix::float3& sc, float exp) :
            base_material("Blinn-Phong", context, material_source_path, closest_hit, any_hit), diffuse_color(dc),
            specular_color(sc), exponent(exp)
        {
        }
        void set_variables(optix::GeometryInstance& instance)
        {
            instance["diffuse_color"]->setFloat(diffuse_color);
        }
    };
}