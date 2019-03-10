//
// Created by Göksu Güvendiren on 2019-03-05.
//

#pragma once

#include <optix/optixu/optixpp_namespace.h>
#include <include/light_sources/point.hpp>
#include <include/materials/base_material.hpp>
#include <include/light_sources/area.hpp>

#include <map>
#include <memory>

namespace grpt
{
    class scene;
    class parallelogram;

    struct material_data
    {
        std::string type;
        std::string name;

        std::string source;
        std::string closest_hit;
        std::string any_hit;

        optix::float3 diffuse_color;
        optix::float3 specular_color;
        float exponent;
    };

    class ctx
    {
        optix::Context context;
        optix::Program pgram_bounding_box;
        optix::Program pgram_intersection;

        optix::GeometryInstance get_instance(const grpt::parallelogram& pg);


    public:
        ctx();
        ctx(const ctx& rhs) = delete;
        ctx(ctx&& rhs) : context(rhs.context) {}
        ~ctx();

        std::vector<optix::GeometryInstance> gis;
        std::map<std::string, std::unique_ptr<grpt::base_material>> materials;
//        std::map<std::string, optix::Material> materials;

        void init(const grpt::scene& sc);

        void createGeometryGroup(const std::vector<optix::GeometryInstance>& gis, const std::string& name);

        void addPointLight(const std::vector<grpt::point_light>& pls);
        void addAreaLight(const std::vector<grpt::area_light>& als);
        void addParallelogram(grpt::parallelogram pg);
        void addParallelogram(const optix::float3 &anchor, const optix::float3 &o1, const optix::float3 &o2, const std::string& mat_name, const optix::float3 color);
        void addMaterial(const std::string& name, const std::string& sample_name, const std::string& material_source_path,
                         const std::string& closest_hit, const std::string& any_hit = "");
        void addMaterial(const std::string& sample_name, material_data);
        void set_materials(std::map<std::string, std::unique_ptr<grpt::base_material>> mats)
        {
            materials = std::move(mats);
        }
//        void addMaterial(const std::string& name, const std::string& sample_name, const std::string& material_source_path,
//                         const std::string& closest_hit);

        optix::Buffer get_output_buffer();
        optix::Context& get();
    };
}