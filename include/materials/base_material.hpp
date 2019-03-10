//
// Created by Göksu Güvendiren on 2019-03-09.
//

#pragma once

#include <sutil.h>

namespace grpt
{
    class base_material
    {
        std::string name;
        optix::Material material;

    protected:
        base_material(std::string n, optix::Context context, const std::string& material_source_path, const std::string& closest_hit, const std::string& any_hit) : name(std::move(n))
        {
            material = context->createMaterial();
            const char *ptx = sutil::getPtxString( "optixMeshViewer", material_source_path.c_str());
            auto diffuse_ch = context->createProgramFromPTXString( ptx, closest_hit );
            auto diffuse_ah = context->createProgramFromPTXString( ptx, any_hit );
            material->setClosestHitProgram( 0, diffuse_ch );
            material->setAnyHitProgram( 1, diffuse_ah );
        }
    public:
        optix::Material& get() { return material; }
        virtual void set_variables(optix::GeometryInstance& instance) = 0;
    };
}