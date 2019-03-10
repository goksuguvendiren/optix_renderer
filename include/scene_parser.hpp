//
// Created by Göksu Güvendiren on 2019-03-04.
//

#pragma once

#include <string>
#include <fstream>
#include <scene.hpp>
#include <json/json.hpp>
#include "camera.hpp"
#include <materials/blinn_phong.hpp>
#include <shapes/parallelogram.hpp>

#include <optixu_math_namespace.h>

namespace grpt
{
    optix::float3 get_float3(nlohmann::json& j)
    {
        auto a0 = j[0];
        auto a1 = j[1];
        auto a2 = j[2];
        return optix::make_float3(a0, a1, a2);
    }

    class scene_parser
    {
    public:
        grpt::camera load_camera(nlohmann::json& j, unsigned int w, unsigned int h)
        {
            auto eye = get_float3(j["Eye"]);
            auto at  = get_float3(j["Lookat"]);
            auto up  = get_float3(j["Up"]);

            return grpt::camera{eye, at, up, optix::Matrix4x4::identity(), w, h};
        }

        std::vector<grpt::point_light> load_point_light(nlohmann::json& j)
        {
//            std::cout << j.dump(4) << std::endl;
            std::vector<grpt::point_light> pls;
            for (auto pl : j)
            {
                auto pos = get_float3(pl["Position"]);
                auto ems = get_float3(pl["Emission"]);

                pls.emplace_back(pos, ems);
            }

            return pls;
        }

        std::vector<grpt::area_light> load_area_light(nlohmann::json& j)
        {
            std::vector<grpt::area_light> als;

            std::cerr << j.dump(4) << '\n';
            for (auto al : j)
            {
                auto corner = get_float3(al["Corner"]);
                auto v1 = get_float3(al["V1"]);
                auto v2 = get_float3(al["V2"]);
                auto ems = get_float3(al["Emission"]);

                als.emplace_back(corner, v1, v2, ems);
            }
            return als;
        }

        std::vector<grpt::parallelogram> load_parallelogram(nlohmann::json& j)
        {
            std::vector<grpt::parallelogram> pgs;

            for (auto pg : j["Parallelograms"])
            {
                auto anchor = get_float3(pg["anchor"]);
                auto v1 = get_float3(pg["v1"]);
                auto v2 = get_float3(pg["v2"]);
                pgs.emplace_back(anchor, v1, v2, pg["material"], optix::make_float3(0.43f));
            }

            return pgs;
        }

//        std::vector<grpt::triangle_mesh> load_triangle_meshes(nlohmann::json& j)
//        {
//            std::vector<grpt::triangle_mesh> tms;
//
//            for (auto tm : j["TriangleMeshes"])
//            {
//                auto anchor = get_float3(tm["anchor"]);
//                auto v1 = get_float3(tm["v1"]);
//                auto v2 = get_float3(tm["v2"]);
//                tms.emplace_back(anchor, v1, v2, tm["material"], optix::make_float3(0.43f));
//            }
//
//            return tms;
//        }

        std::vector<material_data> load_materials(nlohmann::json& j)
        {
            std::vector<material_data> mats;

            for (auto m : j)
            {
                if (m["Type"] == "Blinn-Phong")
                {
                    material_data data;
                    data.name = m["Name"];
                    data.type = m["Type"];
                    data.source = m["Source"];
                    data.closest_hit = m["ClosestHit"];
                    data.any_hit = m["AnyHit"];

                    data.diffuse_color = get_float3(m["DiffuseColor"]);
                    data.specular_color = get_float3(m["SpecularColor"]);
                    data.exponent = m["Exponent"];

                    mats.push_back(std::move(data));
                }
                else
                    throw std::runtime_error("Unknown material type!");
            }

            return mats;

        }

        grpt::scene LoadFromJson(const std::string& json_path)
        {
            std::ifstream i(json_path);
            nlohmann::json j;
            i >> j;

            scene sc;
            sc.width  = j["Plane"]["Width"];
            sc.height = j["Plane"]["Height"];
            sc.add_camera(load_camera(j["Camera"], sc.width, sc.height));
            auto pls = load_point_light(j["Lights"]["PointLights"]);
            sc.add_point_light(load_point_light(j["Lights"]["PointLights"]));
            sc.add_area_light(load_area_light(j["Lights"]["AreaLights"]));
            sc.spp    = j["SPP"];
            sc.bad_color = get_float3(j["BadColor"]);
            sc.bg_color  = get_float3(j["BackgroundColor"]);
            sc.sample_name = "optixPathTracer";
            sc.scene_eps   = j["SceneEpsilon"];
            sc.ray_gen_prog_path = j["RayGenerationProgramFile"];
            sc.ray_gen_prog = j["RayGenerationProgram"];
            sc.exception_prog = j["ExceptionProgram"];
            sc.miss_prog_path = j["MissProgramFile"];
            sc.miss_prog = j["MissProgram"];

            sc.init();

            //TODO : load vertices and indices from the json.
            sc.add_materials(load_materials(j["Materials"]));
            sc.add_geometry(load_parallelogram(j["Geometry"]));
//            sc.add_geometry(load_triangle_meshes(j["Geometry"]));

            return sc;
        }
    };
}
