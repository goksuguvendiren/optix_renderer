//
// Created by Göksu Güvendiren on 2019-03-05.
//

#include <ctx.hpp>
#include <cmath>
#include <include/scene.hpp>
#include <sutil.h>
#include <include/ctx.hpp>


grpt::ctx::ctx()
{
    context = optix::Context::create();
}

grpt::ctx::~ctx()
{
    if( context )
    {
        context->destroy();
        context = 0;
    }
}


optix::Buffer grpt::ctx::get_output_buffer()
{
    return context[ "output_buffer" ]->getBuffer();
}

optix::Context& grpt::ctx::get()
{
    return context;
}

void grpt::ctx::init(const grpt::scene& sc)
{
    context->setRayTypeCount( 2 );
    context->setEntryPointCount( 1 );
    context->setStackSize( 1800 );

    context[ "scene_epsilon"                  ]->setFloat( 1.e-3f );
    context[ "pathtrace_ray_type"             ]->setUint( 0u );
    context[ "pathtrace_shadow_ray_type"      ]->setUint( 1u );
    context[ "rr_begin_depth"                 ]->setUint( sc.rr_begin_depth );

    optix::Buffer buffer = sutil::createOutputBuffer( context, RT_FORMAT_FLOAT4, sc.width, sc.height, false );
    context["output_buffer"]->set( buffer );

    // Setup programs
    const char *ptx = sutil::getPtxString( sc.sample_name.c_str(), "../src/ray_generators/ray_tracer.cu" );
//    const char *ptx = sutil::getPtxString( sc.sample_name.c_str(), "../optixPathTracer.cu" );
    context->setRayGenerationProgram( 0, context->createProgramFromPTXString( ptx, "pinhole_camera" ) );
    context->setExceptionProgram( 0, context->createProgramFromPTXString( ptx, "exception" ) );
    context->setMissProgram( 0, context->createProgramFromPTXString( ptx, "miss" ) );

    context[ "sqrt_num_samples" ]->setUint( std::sqrt(sc.spp) );
    context[ "bad_color"        ]->setFloat( sc.bad_color ); // Super magenta to make sure it doesn't get averaged out in the progressive rendering.
    context[ "bg_color"         ]->setFloat( sc.bg_color );
}
