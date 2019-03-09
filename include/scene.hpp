//
// Created by Göksu Güvendiren on 2019-03-04.
//

#pragma once

#include <cstring>

#include <ctx.hpp>
#include <camera.hpp>
#include <light_sources/point.hpp>
#include <light_sources/area.hpp>
#include <shapes/triangle_mesh.hpp>

namespace grpt
{
    class scene
    {
        std::vector<camera> cameras;
        std::vector<optix::float3> vertices;
        std::vector<unsigned int>  indices;
        std::vector<grpt::parallelogram>  parallelograms;
        std::vector<grpt::triangle_mesh>  triangle_meshes;

    public:
        scene() = default;
        scene(scene&&) = default;

        void add_camera(grpt::camera cam) { cameras.push_back(std::move(cam)); }
        void add_point_light(grpt::point_light&& l) { point_ls.push_back(l); }
        void add_point_light(std::vector<grpt::point_light> pls) { point_ls = std::move(pls); }
        void add_area_light(grpt::area_light&& l) { area_ls.push_back(l); }
        void add_geometry(std::vector<grpt::parallelogram> pgs);
        void add_geometry(std::vector<grpt::triangle_mesh> pgs);

        std::vector<point_light> point_ls;
        std::vector<area_light> area_ls;

        std::string ray_gen_prog_path;
        std::string ray_gen_prog;
        std::string exception_prog;
        std::string miss_prog_path;
        std::string miss_prog;

        unsigned int width;
        unsigned int height;
        unsigned int spp;
        unsigned int rr_begin_depth;
        float        scene_eps;

        optix::float3 bad_color;
        optix::float3 bg_color;

        grpt::ctx cont;
        optix::Context& ctx() { return cont.get(); };

        grpt::camera& get_camera() { return cameras[0]; }

        std::string sample_name;

        void init();
    };
}