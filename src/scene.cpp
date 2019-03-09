//
// Created by Göksu Güvendiren on 2019-03-06.
//

#include <scene.hpp>
#include <include/scene.hpp>
#include <shapes/parallelogram.hpp>
#include <OptiXMesh.h>

void grpt::scene::init()
{
    cont.init(*this);

    cont.addPointLight(point_ls);
    cont.addAreaLight(area_ls);

    cont.addMaterial("Diffuse", sample_name, "../src/shading_models/lambertian.cu", "diffuse", "shadow");
    cont.addMaterial("Diffuse_light", sample_name, "../src/shading_models/lambertian.cu", "diffuseEmitter");
}

void grpt::scene::add_geometry(std::vector<grpt::parallelogram> pgs)
{
    parallelograms = std::move(pgs);

    for (auto& pg : parallelograms)
    {
        cont.addParallelogram(pg);
    }

    std::string mesh_file = std::string( sutil::samplesDir() ) + "/data/cow.obj";

//    OptiXMesh mesh;
//    mesh.context = cont.get();
//    loadMesh( mesh_file, mesh );

    grpt::triangle_mesh mesh(ctx(), mesh_file);
//    aabb.set( mesh.bbox_min, mesh.bbox_max );

    optix::GeometryGroup geometry_group = cont.get()->createGeometryGroup();
    geometry_group->addChild( mesh.geom_instance );
    geometry_group->setAcceleration( cont.get()->createAcceleration( "Trbvh" ) );
    cont.get()[ "top_object"   ]->set( geometry_group );
    cont.get()[ "top_shadower" ]->set( geometry_group );

//    cont.createGeometryGroup(cont.gis, "top_shadower");
//    cont.createGeometryGroup(cont.gis, "top_object");
}

void grpt::scene::add_geometry(std::vector<grpt::triangle_mesh> pgs)
{
    triangle_meshes = std::move(pgs);
}
