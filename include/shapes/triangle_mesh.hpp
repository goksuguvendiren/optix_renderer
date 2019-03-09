//
// Created by Göksu Güvendiren on 2019-03-08.
//

#pragma once

#include <utils.hpp>
#include <optix_world.h>
#include <tinyobjloader/tiny_obj_loader.h>

namespace grpt
{
    struct MaterialParams
    {
        std::string         name;

        std::string         Kd_map;
        float               Kd[3];
        float               Ks[3];
        float               Kr[3];
        float               Ka[3];
        float               exp;

    };

    class triangle_mesh
    {
    public:
        struct mesh
        {
            struct Buffers
            {
                optix::Buffer tri_indices;
                optix::Buffer mat_indices;
                optix::Buffer positions;
                optix::Buffer normals;
                optix::Buffer texcoords;
            } buffers;

            int32_t             num_vertices;   // Number of triangle vertices
            float*              positions;      // Triangle vertex positions (len num_vertices)

            bool                has_normals;    //
            float*              normals;        // Triangle normals (len 0 or num_vertices)

            bool                has_texcoords;  //
            float*              texcoords;      // Triangle UVs (len 0 or num_vertices)


            int32_t             num_triangles;  // Number of triangles
            int32_t*            tri_indices;    // Indices into positions, normals, texcoords
            int32_t*            mat_indices;    // Indices into mat_params (len num_triangles)

            float               bbox_min[3];    // Scene BBox
            float               bbox_max[3];    //

            int32_t             num_materials;
            MaterialParams*     mat_params;     // Material params

            std::vector<tinyobj::shape_t>       m_shapes;
            std::vector<tinyobj::material_t>    m_materials;

            mesh(optix::Context& context, const std::string& filename);
            bool check_validity();
            void print_mesh_info( std::ostream& out );
            void load(const std::string& filename);
            void load_xform(const float* xform);
        } m;

    void load_obj_file(const std::string& filename, const optix::Matrix4x4& xform);
    void unmap();
        // Input
        optix::Context               context;       // required
        optix::Material              material;      // optional single matl override

        optix::Program               intersection;  // optional
        optix::Program               bounds;        // optional

        optix::Program               closest_hit;   // optional multi matl override
        optix::Program               any_hit;       // optional

        // Output
        optix::GeometryInstance      geom_instance;
        optix::float3                bbox_min;
        optix::float3                bbox_max;

        int                          num_triangles;

        triangle_mesh(optix::Context cont, const std::string& filename, const optix::Matrix4x4&   load_xform = optix::Matrix4x4::identity());
    };
}