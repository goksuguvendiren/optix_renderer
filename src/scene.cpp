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

//    cont.addMaterial("Blinn-Phong", sample_name, "../src/shading_models/lambertian.cu", "diffuse", "shadow");
//    cont.addMaterial("Diffuse_light", sample_name, "../src/shading_models/lambertian.cu", "diffuseEmitter");
}

void grpt::scene::add_geometry(std::vector<grpt::parallelogram> pgs)
{
    parallelograms = std::move(pgs);

    for (auto& pg : parallelograms)
    {
        cont.addParallelogram(pg);
    }

    cont.createGeometryGroup(cont.gis, "top_shadower");
    cont.createGeometryGroup(cont.gis, "top_object");
}

void grpt::scene::add_geometry(std::vector<grpt::triangle_mesh> pgs)
{
    triangle_meshes = std::move(pgs);
}
