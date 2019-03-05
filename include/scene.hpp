//
// Created by Göksu Güvendiren on 2019-03-04.
//

#pragma once

namespace grpt
{
    class scene
    {
        std::vector<camera> cameras;

    public:
        void add_camera(grpt::camera&& cam) { cameras.push_back(std::move(cam)); }
        void add_point_light(grpt::point_light&& l) { point_ls.push_back(std::move(l)); }
        void add_area_light(grpt::area_light&& l) { area_ls.push_back(std::move(l)); }
        std::vector<point_light> point_ls;
        std::vector<area_light> area_ls;

    };
}