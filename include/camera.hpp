//
// Created by Göksu Güvendiren on 2019-03-02.
//

#pragma once

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

namespace grpt
{
    class camera
    {
// Camera state
        optix::float3         camera_up;
        optix::float3         camera_lookat;
        optix::float3         camera_eye;
        optix::Matrix4x4      camera_rotate;
        bool           camera_changed = true;

        uint32_t       width  = 512;
        uint32_t       height = 512;

        const float fov  = 35.0f;
        float aspect_ratio;

        //TODO: read the fov from the json file.

    public:
        camera() : aspect_ratio(width / (float)height)
        {
            camera_eye    = optix::make_float3( 278.0f, 273.0f, -900.0f );
            camera_lookat = optix::make_float3( 278.0f, 273.0f,    0.0f );
            camera_up     = optix::make_float3(   0.0f,   1.0f,    0.0f );

            camera_rotate  = optix::Matrix4x4::identity();
        }

        camera(const optix::float3& eye, const optix::float3& lookat, const optix::float3& up, const optix::Matrix4x4& rotate, unsigned int w, unsigned int h) : camera_up(up), camera_lookat(lookat), camera_eye(eye), camera_rotate(rotate), width(w), height(h)
        {
            aspect_ratio = width / (float)height;
        }

        int         frame_number = 0;

        optix::float3    up() const { return camera_up; }
        optix::float3    lookat() const { return camera_lookat; }
        optix::float3    eye() const { return camera_eye; }
        optix::Matrix4x4 rotate() const { return camera_rotate; }

//        using uvw = std::tuple<optix::float3, optix::float3, optix::float3>;
        std::tuple<optix::float3, optix::float3, optix::float3> Update(const optix::float3& eye, const optix::float3& lookat, const optix::float3& up, const optix::Matrix4x4& rotate);
    };
}