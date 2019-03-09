//
// Created by Göksu Güvendiren on 2019-03-05.
//

#include <ctx.hpp>
#include <cmath>
#include <include/scene.hpp>
#include <sutil.h>
#include <include/ctx.hpp>
#include <cstring>
#include <vector>
#include <include/shapes/parallelogram.hpp>

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

    context[ "scene_epsilon"                  ]->setFloat( sc.scene_eps );
    context[ "pathtrace_ray_type"             ]->setUint( 0u );
    context[ "pathtrace_shadow_ray_type"      ]->setUint( 1u );
    context[ "rr_begin_depth"                 ]->setUint( sc.rr_begin_depth );

    optix::Buffer buffer = sutil::createOutputBuffer( context, RT_FORMAT_FLOAT4, sc.width, sc.height, false );
    context["output_buffer"]->set( buffer );

    // Setup programs
    const char *ptx = sutil::getPtxString( sc.sample_name.c_str(), sc.ray_gen_prog_path.c_str() );
//    const char *ptx = sutil::getPtxString( sc.sample_name.c_str(), "../optixPathTracer.cu" );
    context->setRayGenerationProgram( 0, context->createProgramFromPTXString( ptx, sc.ray_gen_prog.c_str() ) );
    context->setExceptionProgram( 0, context->createProgramFromPTXString( ptx, sc.exception_prog.c_str() ) );
    if (sc.miss_prog_path.size() > 0)
        ptx = sutil::getPtxString(sc.sample_name.c_str(), sc.miss_prog_path.c_str());
    context->setMissProgram( 0, context->createProgramFromPTXString( ptx, sc.miss_prog.c_str() ) );


    context[ "sqrt_num_samples" ]->setUint( std::sqrt(sc.spp) );
    context[ "bad_color"        ]->setFloat( sc.bad_color ); // Super magenta to make sure it doesn't get averaged out in the progressive rendering.
    context[ "bg_color"         ]->setFloat( sc.bg_color );

    ptx = sutil::getPtxString( sc.sample_name.c_str(), "../src/shapes/parallelogram.cu" );
    pgram_bounding_box = context->createProgramFromPTXString( ptx, "bounds" );
    pgram_intersection = context->createProgramFromPTXString( ptx, "intersect" );
}

void grpt::ctx::addPointLight(const std::vector<grpt::point_light> &pls)
{
    if (pls.size() > 0)
    {
        optix::Buffer plight_buffer = context->createBuffer(RT_BUFFER_INPUT);
        plight_buffer->setFormat(RT_FORMAT_USER);
        plight_buffer->setElementSize(sizeof(grpt::point_light));
        plight_buffer->setSize(pls.size());
        memcpy(plight_buffer->map(), pls.data(), sizeof(pls[0]) * pls.size());
        plight_buffer->unmap();
        context["point_lights"]->setBuffer(plight_buffer);
    }
}

void grpt::ctx::addAreaLight(const std::vector<grpt::area_light> &als)
{
//    grpt::area_light& light = scene.area_ls[0];
    if (als.size() > 0)
    {
        optix::Buffer light_buffer = context->createBuffer(RT_BUFFER_INPUT);
        light_buffer->setFormat(RT_FORMAT_USER);
        light_buffer->setElementSize(sizeof(grpt::area_light));
        light_buffer->setSize(als.size());
        memcpy(light_buffer->map(), &als[0], sizeof(als[0]));
        light_buffer->unmap();
        context["area_lights"]->setBuffer(light_buffer);
    }
}

optix::GeometryInstance grpt::ctx::get_instance(const grpt::parallelogram& pg)
{
    optix::Geometry parallelogram = context->createGeometry();
    parallelogram->setPrimitiveCount( 1u );
    parallelogram->setIntersectionProgram( pgram_intersection );
    parallelogram->setBoundingBoxProgram( pgram_bounding_box );

    optix::float3 normal = normalize( cross( pg.off1, pg.off2 ) );
    float d = dot( normal, pg.anchor );
    optix::float4 plane = make_float4( normal, d );

    optix::float3 v1 = pg.off1 / dot( pg.off1, pg.off1 );
    optix::float3 v2 = pg.off2 / dot( pg.off2, pg.off2 );

    parallelogram["plane"]->setFloat( plane );
    parallelogram["anchor"]->setFloat( pg.anchor );
    parallelogram["v1"]->setFloat( v1 );
    parallelogram["v2"]->setFloat( v2 );

    optix::GeometryInstance gi = context->createGeometryInstance();
    gi->setGeometry(parallelogram);
    return gi;
}


void set_material(optix::GeometryInstance& gi, optix::Material material, const std::string& color_name, const optix::float3& color)
{
    gi->addMaterial(material);
    gi[color_name]->setFloat(color);
}

void set_material(optix::GeometryInstance& gi, optix::Material material, const std::vector<std::pair<std::string, optix::float3>>& colors)
{
    gi->addMaterial(material);
    for (auto& color : colors)
        gi[color.first]->setFloat(color.second);
}

void grpt::ctx::addParallelogram(const optix::float3 &anchor, const optix::float3 &o1, const optix::float3 &o2, const std::string& mat_name, const optix::float3 color)
{
    auto pg = grpt::parallelogram(anchor, o1, o2, mat_name, color);
    auto instance = get_instance(pg);

    auto mat = materials[mat_name];
    set_material(instance, mat, "diffuse_color", color);
//    set_material(instance, mat, {{"diffuse_color", color}});

    gis.push_back(instance);
}

void grpt::ctx::addParallelogram(grpt::parallelogram pg)
{
    auto instance = get_instance(pg);

    auto mat = materials[pg.material];
    set_material(instance, mat, "diffuse_color", pg.color);
//    set_material(instance, mat, {{"diffuse_color", pg.color}});
    gis.push_back(instance);
}

void grpt::ctx::addMaterial(const std::string& name, const std::string& sample_name, const std::string& material_source_path,
                            const std::string& closest_hit, const std::string& any_hit)
{
    auto mat = context->createMaterial();
    const char *ptx = sutil::getPtxString( sample_name.c_str(), material_source_path.c_str());
    auto diffuse_ch = context->createProgramFromPTXString( ptx, closest_hit );
    auto diffuse_ah = context->createProgramFromPTXString( ptx, any_hit );
    mat->setClosestHitProgram( 0, diffuse_ch );
    mat->setAnyHitProgram( 1, diffuse_ah );

    materials.insert({name, mat});
}

void
grpt::ctx::addMaterial(const std::string &name, const std::string &sample_name, const std::string &material_source_path,
                       const std::string &closest_hit)
{
    auto mat = context->createMaterial();
    const char *ptx = sutil::getPtxString( sample_name.c_str(), material_source_path.c_str());
    auto diffuse_ch = context->createProgramFromPTXString( ptx, closest_hit );
    mat->setClosestHitProgram( 0, diffuse_ch );

    materials.insert({name, mat});
}

void grpt::ctx::createGeometryGroup(const std::vector<optix::GeometryInstance>& gis, const std::string& name)
{
    optix::GeometryGroup group = context->createGeometryGroup(gis.begin(), gis.end());
    group->setAcceleration(context->createAcceleration("Trbvh"));
    context[name]->set(group);
}