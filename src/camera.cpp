//
// Created by Göksu Güvendiren on 2019-03-02.
//

#include <camera.hpp>
#include <app.hpp>

#include <tuple>


void calculateCameraVariables(const optix::float3& eye, const optix::float3& lookat, const optix::float3& up,
                              float  fov, float  aspect_ratio,
                              optix::float3& U, optix::float3& V, optix::float3& W, bool fov_is_vertical)
{
    float ulen, vlen, wlen;
    W = lookat - eye; // Do not normalize W -- it implies focal length

    wlen = optix::length( W );
    U = normalize( cross( W, up ) );
    V = normalize( cross( U, W  ) );

    if ( fov_is_vertical )
    {
        vlen = wlen * tanf( 0.5f * fov * M_PIf / 180.0f );
        V *= vlen;
        ulen = vlen * aspect_ratio;
        U *= ulen;
    }
    else
        {
        ulen = wlen * tanf( 0.5f * fov * M_PIf / 180.0f );
        U *= ulen;
        vlen = ulen / aspect_ratio;
        V *= vlen;
    }
}

std::tuple<optix::float3, optix::float3, optix::float3> grpt::camera::Update(const optix::float3& eye, const optix::float3& lookat, const optix::float3& up, const optix::Matrix4x4& rotate)
{
    optix::float3 camera_u, camera_v, camera_w;
    calculateCameraVariables(eye, lookat, up, fov, aspect_ratio,
            camera_u, camera_v, camera_w, /*fov_is_vertical*/ true );

    const optix::Matrix4x4 frame = optix::Matrix4x4::fromBasis(
            normalize( camera_u ),
            normalize( camera_v ),
            normalize( -camera_w ),
            lookat);
    const optix::Matrix4x4 frame_inv = frame.inverse();
    // Apply camera rotation twice to match old SDK behavior
    const optix::Matrix4x4 trans     = frame*camera_rotate*camera_rotate*frame_inv;

    camera_eye    = make_float3( trans*make_float4( eye,    1.0f ) );
    camera_lookat = make_float3( trans*make_float4( camera_lookat, 1.0f ) );
    camera_up     = make_float3( trans*make_float4( camera_up,     0.0f ) );

    calculateCameraVariables(
            camera_eye, camera_lookat, camera_up, fov, aspect_ratio,
            camera_u, camera_v, camera_w, true );

    camera_rotate = optix::Matrix4x4::identity();

    if( camera_changed ) // reset accumulation
        frame_number = 1;

    camera_changed = false;

    return {camera_u, camera_v, camera_w};
}