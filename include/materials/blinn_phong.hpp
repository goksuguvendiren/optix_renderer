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
        blinn_phong(optix::Context context, const std::string& material_source_path, const std::string& closest_hit, const std::string& any_hit) :
            base_material("Blinn-Phong", context, material_source_path, closest_hit, any_hit)
        {
            diffuse_color = optix::make_float3(0.3f);
        }
        void set_variables(optix::GeometryInstance& instance)
        {
            instance["diffuse_color"]->setFloat(diffuse_color);
        }
    };
}