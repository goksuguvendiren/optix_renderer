//
// Created by Göksu Güvendiren on 2019-03-08.
//

#include <shapes/triangle_mesh.hpp>
#include <include/shapes/triangle_mesh.hpp>
#include <tinyobjloader/tiny_obj_loader.h>

grpt::triangle_mesh::mesh::mesh(optix::Context& context, const std::string& filename)
{
    std::string error;
    auto ret = tinyobj::LoadObj(m_shapes, m_materials, error, filename.c_str(), utils::directoryOfFilePath( filename ).c_str());

    if( !error.empty() )
        std::cerr << error << std::endl;

    if( !ret )
        throw std::runtime_error( "MeshLoader: " + error );

    uint64_t num_groups_with_normals   = 0;
    uint64_t num_groups_with_texcoords = 0;
    for( std::vector<tinyobj::shape_t>::const_iterator it = m_shapes.begin();
         it < m_shapes.end();
         ++it )
    {
        const tinyobj::shape_t & shape = *it;

        num_triangles += static_cast<int32_t>(shape.mesh.indices.size()) / 3;
        num_vertices  += static_cast<int32_t>(shape.mesh.positions.size()) / 3;

        if( !shape.mesh.normals.empty() )
            ++num_groups_with_normals;

        if( !shape.mesh.texcoords.empty() )
            ++num_groups_with_texcoords;
    }

    //
    // We ignore normals and texcoords unless they are present for all shapes
    //

    if( num_groups_with_normals != 0 )
    {
        if( num_groups_with_normals != m_shapes.size() )
            std::cerr << "MeshLoader - WARNING: mesh '" << filename
                      << "' has normals for some groups but not all.  "
                      << "Ignoring all normals." << std::endl;
        else
            has_normals = true;

    }

    if( num_groups_with_texcoords != 0 )
    {
        if( num_groups_with_texcoords != m_shapes.size() )
            std::cerr << "MeshLoader - WARNING: mesh '" << filename
                      << "' has texcoords for some groups but not all.  "
                      << "Ignoring all texcoords." << std::endl;
        else
            has_texcoords = true;
    }

    num_materials = (int32_t) m_materials.size();

    // setup mesh loader inputs
    buffers.tri_indices = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, num_triangles);
    buffers.mat_indices = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_INT, num_triangles );
    buffers.positions   = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, num_vertices );
    buffers.normals     = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3,
                                                 has_normals ? num_vertices : 0);
    buffers.texcoords   = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT2,
                                                 has_texcoords ? num_vertices : 0);

    tri_indices = reinterpret_cast<int32_t*>( buffers.tri_indices->map() );
    mat_indices = reinterpret_cast<int32_t*>( buffers.mat_indices->map() );
    positions   = reinterpret_cast<float*>  ( buffers.positions->map() );
    normals     = reinterpret_cast<float*>  ( has_normals   ? buffers.normals->map()   : 0 );
    texcoords   = reinterpret_cast<float*>  ( has_texcoords ? buffers.texcoords->map() : 0 );

    mat_params = new MaterialParams[ num_materials ];
}

bool grpt::triangle_mesh::mesh::check_validity()
{
    if( num_vertices  == 0 )
    {
        std::cerr << "Mesh not valid: num_vertices = 0" << std::endl;
        return false;
    }
    if( positions == 0 )
    {
        std::cerr << "Mesh not valid: positions = NULL" << std::endl;
        return false;
    }
    if( num_triangles == 0 )
    {
        std::cerr << "Mesh not valid: num_triangles = 0" << std::endl;
        return false;
    }
    if( tri_indices == 0 )
    {
        std::cerr << "Mesh not valid: tri_indices = NULL" << std::endl;
        return false;
    }
    if( mat_indices == 0 )
    {
        std::cerr << "Mesh not valid: mat_indices = NULL" << std::endl;
        return false;
    }
    if( has_normals && !normals )
    {
        std::cerr << "Mesh has normals, but normals is NULL" << std::endl;
        return false;
    }
    if( has_texcoords && !texcoords )
    {
        std::cerr << "Mesh has texcoords, but texcoords is NULL" << std::endl;
        return false;
    }
    if ( num_materials == 0 )
    {
        std::cerr << "Mesh not valid: num_materials = 0" << std::endl;
        return false;
    }
    if ( mat_params == 0 )
    {
        std::cerr << "Mesh not valid: mat_params = 0" << std::endl;
        return false;
    }

    return true;
}

void grpt::triangle_mesh::mesh::print_mesh_info(std::ostream &out)
{
    out << "Mesh:" << std::endl
        << "\tnum vertices : " << num_vertices << std::endl
        << "\thas normals  : " << has_normals << std::endl
        << "\thas texcoords: " << has_texcoords << std::endl
        << "\tnum triangles: " << num_triangles << std::endl
        << "\tnum materials: " << num_materials << std::endl
        << "\tbbox min     : ( " << bbox_min[0] << ", "
        << bbox_min[1] << ", "
        << bbox_min[2] << " )"
        << std::endl
        << "\tbbox max     : ( " << bbox_max[0] << ", "
        << bbox_max[1] << ", "
        << bbox_max[2] << " )"
        << std::endl;
}

void grpt::triangle_mesh::mesh::load(const std::string& filename)
{
    uint32_t vrt_offset = 0;
    uint32_t tri_offset = 0;
    for( std::vector<tinyobj::shape_t>::const_iterator it = m_shapes.begin();
         it < m_shapes.end();
         ++it )
    {
        const tinyobj::shape_t & shape = *it;
        for( uint64_t i = 0; i < shape.mesh.positions.size() / 3; ++i )
        {
            const float x = shape.mesh.positions[i*3+0];
            const float y = shape.mesh.positions[i*3+1];
            const float z = shape.mesh.positions[i*3+2];

            bbox_min[0] = std::min<float>( bbox_min[0], x );
            bbox_min[1] = std::min<float>( bbox_min[1], y );
            bbox_min[2] = std::min<float>( bbox_min[2], z );

            bbox_max[0] = std::max<float>( bbox_max[0], x );
            bbox_max[1] = std::max<float>( bbox_max[1], y );
            bbox_max[2] = std::max<float>( bbox_max[2], z );

            positions[ vrt_offset*3 + i*3+0 ] = x;
            positions[ vrt_offset*3 + i*3+1 ] = y;
            positions[ vrt_offset*3 + i*3+2 ] = z;
        }

        if( has_normals )
            for( uint64_t i = 0; i < shape.mesh.normals.size(); ++i )
                normals[ vrt_offset*3 + i ] = shape.mesh.normals[i];

        if( has_texcoords )
            for( uint64_t i = 0; i < shape.mesh.texcoords.size(); ++i )
                texcoords[ vrt_offset*2 + i ] = shape.mesh.texcoords[i];

        for( uint64_t i = 0; i < shape.mesh.indices.size() / 3; ++i )
        {
            tri_indices[ tri_offset*3 + i*3+0 ] = shape.mesh.indices[i*3+0] + vrt_offset;
            tri_indices[ tri_offset*3 + i*3+1 ] = shape.mesh.indices[i*3+1] + vrt_offset;
            tri_indices[ tri_offset*3 + i*3+2 ] = shape.mesh.indices[i*3+2] + vrt_offset;
            mat_indices[ tri_offset + i ] = shape.mesh.material_ids[i] >= 0 ?
                                                 shape.mesh.material_ids[i]      :
                                                 0;
        }

        vrt_offset += static_cast<uint32_t>(shape.mesh.positions.size()) / 3;
        tri_offset += static_cast<uint32_t>(shape.mesh.indices.size()) / 3;
    }

    for( uint64_t i = 0; i < m_materials.size(); ++i )
    {
        MaterialParams mps;

        mps.name   = m_materials[i].name;
        mps.Kd_map = m_materials[i].diffuse_texname.empty() ? "" :
                            utils::directoryOfFilePath( filename ) + m_materials[i].diffuse_texname;

        mps.Kd[0]  = m_materials[i].diffuse[0];
        mps.Kd[1]  = m_materials[i].diffuse[1];
        mps.Kd[2]  = m_materials[i].diffuse[2];

        mps.Ks[0]  = m_materials[i].specular[0];
        mps.Ks[1]  = m_materials[i].specular[1];
        mps.Ks[2]  = m_materials[i].specular[2];

        mps.Ka[0]  = m_materials[i].ambient[0];
        mps.Ka[1]  = m_materials[i].ambient[1];
        mps.Ka[2]  = m_materials[i].ambient[2];

        mps.Kr[0]  = m_materials[i].specular[0];
        mps.Kr[1]  = m_materials[i].specular[1];
        mps.Kr[2]  = m_materials[i].specular[2];

        mps.exp    = m_materials[i].shininess;

        mat_params[i] = mps;
    }

}

void grpt::triangle_mesh::mesh::load_xform(const float* load_xform)
{
    if( !load_xform )
        return;

    bool have_matrix = false;
    for( int32_t i = 0; i < 16; ++i )
        if( load_xform[i] != 0.0f )
            have_matrix = true;

    if( have_matrix )
    {
        bbox_min[0] = bbox_min[1] = bbox_min[2] =  1e16f;
        bbox_max[0] = bbox_max[1] = bbox_max[2] = -1e16f;

        optix::Matrix4x4 mat( load_xform );

        optix::float3* positions = reinterpret_cast<optix::float3*>( positions );
        for( int32_t i = 0; i < num_vertices; ++i )
        {
            const optix::float3 v = optix::make_float3( mat*optix::make_float4( positions[i], 1.0f ) );
            positions[i] = v;
            bbox_min[0] = std::min<float>( bbox_min[0], v.x );
            bbox_min[1] = std::min<float>( bbox_min[1], v.y );
            bbox_min[2] = std::min<float>( bbox_min[2], v.z );
            bbox_max[0] = std::max<float>( bbox_max[0], v.x );
            bbox_max[1] = std::max<float>( bbox_max[1], v.y );
            bbox_max[2] = std::max<float>( bbox_max[2], v.z );
        }

        if( has_normals )
        {
            mat = mat.inverse().transpose();
            optix::float3* normals = reinterpret_cast<optix::float3*>( normals );
            for( int32_t i = 0; i < num_vertices; ++i )
                normals[i] = optix::make_float3( mat*optix::make_float4( normals[i], 1.0f ) );
        }
    }

}

grpt::triangle_mesh::triangle_mesh(optix::Context cont, const std::string &filename, const optix::Matrix4x4 &load_xform) : context(cont), m(cont, filename)
{
    load_obj_file(filename, load_xform);
}

void grpt::triangle_mesh::load_obj_file(const std::string &filename, const optix::Matrix4x4 &load_xform)
{
    if (utils::getExtension(filename) != "obj") return;

    if( !m.check_validity() )
    {
        std::cerr << "MeshLoader - ERROR: Attempted to load mesh '" << filename
                  << "' into invalid mesh struct:" << std::endl;
        m.print_mesh_info( std::cerr );
        return;
    }

    m.bbox_min[0] = m.bbox_min[1] = m.bbox_min[2] =  1e16f;
    m.bbox_max[0] = m.bbox_max[1] = m.bbox_max[2] = -1e16f;

//    loadMeshOBJ( m );
    m.load(filename);
    m.load_xform( load_xform.getData() );
}
