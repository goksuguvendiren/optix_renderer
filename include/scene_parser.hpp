//
// Created by Göksu Güvendiren on 2019-03-04.
//

#pragma once

#include <string>
#include <fstream>
#include <scene.hpp>
#include <json/json.hpp>
#include "camera.hpp"

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
        grpt::camera load_camera(nlohmann::json& j)
        {
            auto eye = get_float3(j["Eye"]);
            auto at  = get_float3(j["Lookat"]);
            auto up  = get_float3(j["Up"]);

            return {eye, at, up, optix::Matrix4x4::identity()};
        }

        grpt::point_light load_point_light(nlohmann::json& j)
        {
            auto pos = get_float3(j["Position"]);
            auto ems = get_float3(j["Emission"]);

            return {pos, ems};
        }

        grpt::area_light load_area_light(nlohmann::json& j)
        {
            auto corner = get_float3(j["Corner"]);
            auto v1     = get_float3(j["V1"]);
            auto v2     = get_float3(j["V2"]);
            auto ems    = get_float3(j["Emission"]);

            return {corner, v1, v2, ems};
        }

        grpt::scene LoadFromJson(const std::string& json_path)
        {
            std::ifstream i(json_path);
            nlohmann::json j;
            i >> j;

            scene sc;
            sc.add_camera(load_camera(j["Camera"]));
            sc.add_point_light(load_point_light(j["Lights"][0]["PointLight"]));
            sc.add_area_light(load_area_light(j["Lights"][1]["AreaLight"]));
            sc.width  = j["Plane"]["Width"];
            sc.height = j["Plane"]["Height"];
            sc.spp    = j["SPP"];
            sc.bad_color = get_float3(j["BadColor"]);
            sc.bg_color  = get_float3(j["BackgroundColor"]);
            sc.sample_name = "optixPathTracer";

            sc.init();
            return sc;
        }
    };
}
