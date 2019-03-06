//
// Created by Göksu Güvendiren on 2019-03-05.
//

#pragma once

#include <optix/optixu/optixpp_namespace.h>

namespace grpt
{
    class scene;
    class ctx
    {
        optix::Context context;

    public:
        ctx();
        ctx(const ctx& rhs) = delete;
        ctx(ctx&& rhs) : context(rhs.context) {}
        ~ctx();

        void init(const grpt::scene& sc);

        optix::Buffer get_output_buffer();
        optix::Context& get();
    };
}