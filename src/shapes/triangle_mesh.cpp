//
// Created by Göksu Güvendiren on 2019-03-08.
//

#include <cstring>
#include <shapes/triangle_mesh.hpp>
#include <include/shapes/triangle_mesh.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
#include <optixu/optixu_math_namespace.h>
#include <sutil.h>
#include <Mesh.h>

grpt::triangle_mesh::mesh::mesh(optix::Context& context, const std::string& filename)
{
    // beginning of scanmesh()
    std::string error;
    auto ret = tinyobj::LoadObj(m_shapes, m_materials, error, filename.c_str(), utils::directoryOfFilePath( filename ).c_str());

    if( !error.empty() )
        std::cerr << error << std::endl;

    if( !ret )
        throw std::runtime_error( "MeshLoader: " + error );

    uint64_t num_groups_with_normals   = 0;
    uint64_t num_groups_with_texcoords = 0;

    num_vertices  = 0;
    positions = nullptr;

    has_normals = false;
    normals = nullptr;

    has_texcoords = false;
    texcoords = nullptr;

    num_triangles = 0;
    tri_indices = nullptr;
    mat_indices = nullptr;

    num_materials = 0;
    mat_params = nullptr;

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

    // beginning of setupMeshLoaderInputs

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
        MaterialParams& mps = mat_params[i];

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

        optix::float3* poss = reinterpret_cast<optix::float3*>( positions );
        for( int32_t i = 0; i < num_vertices; ++i )
        {
            const optix::float3 v = optix::make_float3( mat*optix::make_float4( poss[i], 1.0f ) );
            poss[i] = v;
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
            optix::float3* normls = reinterpret_cast<optix::float3*>( normals );
            for( int32_t i = 0; i < num_vertices; ++i )
                normls[i] = optix::make_float3( mat*optix::make_float4( normls[i], 1.0f ) );
        }
    }

}

grpt::triangle_mesh::mesh::mesh()
{
    num_vertices  = 0;
    positions = nullptr;

    has_normals = false;
    normals = nullptr;

    has_texcoords = false;
    texcoords = nullptr;

    num_triangles = 0;
    tri_indices = nullptr;
    mat_indices = nullptr;

    num_materials = 0;
    mat_params = nullptr;
}

void grpt::triangle_mesh::load_obj_file(const std::string &filename, const optix::Matrix4x4 &load_xform)
{
    if (utils::getExtension(filename) != "obj") return;
//    loadMeshOBJ( m );
}

namespace optix {
    float3 make_float3( const float* a )
    {
        return make_float3( a[0], a[1], a[2] );
    }
}

void createMaterialPrograms(
        optix::Context         context,
        bool                   use_textures,
        optix::Program&        closest_hit,
        optix::Program&        any_hit
)
{
    const char *ptx = sutil::getPtxString( NULL, "phong.cu" );

    if( !closest_hit )
        closest_hit = context->createProgramFromPTXString( ptx, use_textures ? "closest_hit_radiance_textured" : "closest_hit_radiance" );
    if( !any_hit )
        any_hit     = context->createProgramFromPTXString( ptx, "any_hit_shadow" );
}

optix::Material createOptiXMaterial(
        optix::Context         context,
        optix::Program         closest_hit,
        optix::Program         any_hit,
        const grpt::MaterialParams&  mat_params,
        bool                   use_textures
)
{
    optix::Material mat = context->createMaterial();
    mat->setClosestHitProgram( 0u, closest_hit );
    mat->setAnyHitProgram( 1u, any_hit ) ;

    if( use_textures )
        mat[ "Kd_map"]->setTextureSampler( sutil::loadTexture( context, mat_params.Kd_map, optix::make_float3(mat_params.Kd) ) );
    else
        mat[ "Kd_map"]->setTextureSampler( sutil::loadTexture( context, "", optix::make_float3(mat_params.Kd) ) );

    mat[ "Kd_mapped" ]->setInt( use_textures  );
    mat[ "Kd"        ]->set3fv( mat_params.Kd );
    mat[ "Ks"        ]->set3fv( mat_params.Ks );
    mat[ "Kr"        ]->set3fv( mat_params.Kr );
    mat[ "Ka"        ]->set3fv( mat_params.Ka );
    mat[ "phong_exp" ]->setFloat( mat_params.exp );

    return mat;
}


optix::Material createOptiXMaterial(
        optix::Context         context,
        optix::Program         closest_hit,
        optix::Program         any_hit,
        const MaterialParams&  mat_params,
        bool                   use_textures
)
{
    optix::Material mat = context->createMaterial();
    mat->setClosestHitProgram( 0u, closest_hit );
    mat->setAnyHitProgram( 1u, any_hit ) ;

    if( use_textures )
        mat[ "Kd_map"]->setTextureSampler( sutil::loadTexture( context, mat_params.Kd_map, optix::make_float3(mat_params.Kd) ) );
    else
        mat[ "Kd_map"]->setTextureSampler( sutil::loadTexture( context, "", optix::make_float3(mat_params.Kd) ) );

    mat[ "Kd_mapped" ]->setInt( use_textures  );
    mat[ "Kd"        ]->set3fv( mat_params.Kd );
    mat[ "Ks"        ]->set3fv( mat_params.Ks );
    mat[ "Kr"        ]->set3fv( mat_params.Kr );
    mat[ "Ka"        ]->set3fv( mat_params.Ka );
    mat[ "phong_exp" ]->setFloat( mat_params.exp );

    return mat;
}

optix::Program createBoundingBoxProgram( optix::Context context )
{
    return context->createProgramFromPTXString( sutil::getPtxString( NULL, "triangle_mesh.cu" ), "mesh_bounds" );
}


optix::Program createIntersectionProgram( optix::Context context )
{
    return context->createProgramFromPTXString( sutil::getPtxString( NULL, "triangle_mesh.cu" ), "mesh_intersect" );
}

struct MeshBuffers
{
    optix::Buffer tri_indices;
    optix::Buffer mat_indices;
    optix::Buffer positions;
    optix::Buffer normals;
    optix::Buffer texcoords;
};


void setupMeshLoaderInputs(
        optix::Context            context,
        MeshBuffers&              buffers,
        Mesh&                     mesh
)
{
    buffers.tri_indices = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_INT3,   mesh.num_triangles );
    buffers.mat_indices = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_INT,    mesh.num_triangles );
    buffers.positions   = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, mesh.num_vertices );
    buffers.normals     = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3,
                                                 mesh.has_normals ? mesh.num_vertices : 0);
    buffers.texcoords   = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT2,
                                                 mesh.has_texcoords ? mesh.num_vertices : 0);

    mesh.tri_indices = reinterpret_cast<int32_t*>( buffers.tri_indices->map() );
    mesh.mat_indices = reinterpret_cast<int32_t*>( buffers.mat_indices->map() );
    mesh.positions   = reinterpret_cast<float*>  ( buffers.positions->map() );
    mesh.normals     = reinterpret_cast<float*>  ( mesh.has_normals   ? buffers.normals->map()   : 0 );
    mesh.texcoords   = reinterpret_cast<float*>  ( mesh.has_texcoords ? buffers.texcoords->map() : 0 );

    mesh.mat_params = new MaterialParams[ mesh.num_materials ];
}

Mesh m2m(const grpt::triangle_mesh::mesh& mesh)
{
    Mesh m;
    m.mat_params = new MaterialParams[mesh.num_materials];

    m.num_vertices = mesh.num_vertices;
    m.positions = mesh.positions;
    m.has_normals = mesh.has_normals;
    m.normals = mesh.normals;
    m.has_texcoords = mesh.has_texcoords;
    m.texcoords = mesh.texcoords;
    m.num_triangles = mesh.num_triangles;
    m.tri_indices = mesh.tri_indices;
    m.mat_indices = mesh.mat_indices;
    m.bbox_min[0] = mesh.bbox_min[0];
    m.bbox_min[1] = mesh.bbox_min[1];
    m.bbox_min[2] = mesh.bbox_min[2];
    m.bbox_max[0] = mesh.bbox_max[0];
    m.bbox_max[1] = mesh.bbox_max[1];
    m.bbox_max[2] = mesh.bbox_max[2];
    m.num_materials = mesh.num_materials;
    m.mat_params->name = mesh.mat_params->name;
    m.mat_params->Kd_map = mesh.mat_params->Kd_map;
    m.mat_params->Kd[0] = mesh.mat_params->Kd[0];
    m.mat_params->Kd[1] = mesh.mat_params->Kd[1];
    m.mat_params->Kd[2] = mesh.mat_params->Kd[2];
    m.mat_params->Ks[0] = mesh.mat_params->Ks[0];
    m.mat_params->Ks[1] = mesh.mat_params->Ks[1];
    m.mat_params->Ks[2] = mesh.mat_params->Ks[2];
    m.mat_params->Kr[0] = mesh.mat_params->Kr[0];
    m.mat_params->Kr[1] = mesh.mat_params->Kr[1];
    m.mat_params->Kr[2] = mesh.mat_params->Kr[2];
    m.mat_params->Ka[0] = mesh.mat_params->Ka[0];
    m.mat_params->Ka[1] = mesh.mat_params->Ka[1];
    m.mat_params->Ka[2] = mesh.mat_params->Ka[2];
    m.mat_params->exp = mesh.mat_params->exp;

    return m;
}

grpt::triangle_mesh::triangle_mesh(optix::Context ctx, const std::string &filename, const optix::Matrix4x4 &load_xform) : context(ctx), m(ctx, filename)
{
    load_obj_file(filename, load_xform);
//    Mesh mesh;
    MeshLoader loader( filename );
    Mesh mesh = m2m(m);

    loader.scanMesh( mesh );

    MeshBuffers buffers;
    setupMeshLoaderInputs( context, buffers, mesh );

    loader.loadMesh( mesh, load_xform.getData() );

    bbox_min      = optix::make_float3( mesh.bbox_min );
    bbox_max      = optix::make_float3( mesh.bbox_max );
    num_triangles = mesh.num_triangles;

    std::vector<optix::Material> optix_materials;
    if( material )
    {
        // Rewrite all mat_indices to point to single override material
        memset( mesh.mat_indices, 0, mesh.num_triangles*sizeof(int32_t) );

        optix_materials.push_back( material );
    }
    else
    {
        bool have_textures = false;
        for( int32_t i = 0; i < mesh.num_materials; ++i )
            if( !mesh.mat_params[i].Kd_map.empty() )
                have_textures = true;

        optix::Program ch = closest_hit;
        optix::Program ah = any_hit;
        createMaterialPrograms( ctx, have_textures, ch, ah );

        for( int32_t i = 0; i < mesh.num_materials; ++i )
            optix_materials.push_back( createOptiXMaterial(
                    ctx,
                    ch,
                    ah,
                    mesh.mat_params[i],
                    have_textures ) );
    }

    optix::Geometry geometry = ctx->createGeometry();
    geometry[ "vertex_buffer"   ]->setBuffer( buffers.positions );
    geometry[ "normal_buffer"   ]->setBuffer( buffers.normals);
    geometry[ "texcoord_buffer" ]->setBuffer( buffers.texcoords );
    geometry[ "material_buffer" ]->setBuffer( buffers.mat_indices);
    geometry[ "index_buffer"    ]->setBuffer( buffers.tri_indices);
    geometry->setPrimitiveCount     ( mesh.num_triangles );
    geometry->setBoundingBoxProgram ( bounds ?
                                      bounds :
                                      createBoundingBoxProgram( ctx ) );
    geometry->setIntersectionProgram( intersection ?
                                      intersection :
                                      createIntersectionProgram( ctx ) );

    geom_instance = ctx->createGeometryInstance(
            geometry,
            optix_materials.begin(),
            optix_materials.end()
    );

    buffers.tri_indices->unmap();
    buffers.mat_indices->unmap();
    buffers.positions->unmap();
    if( mesh.has_normals )
        buffers.normals->unmap();
    if( mesh.has_texcoords)
        buffers.texcoords->unmap();

    mesh.tri_indices = nullptr;
    mesh.mat_indices = nullptr;
    mesh.positions   = nullptr;
    mesh.normals     = nullptr;
    mesh.texcoords   = nullptr;

    delete[] mesh.mat_params;
    mesh.mat_params = nullptr;

//    unmap();
}

void grpt::triangle_mesh::unmap()
{
    m.buffers.tri_indices->unmap();
    m.buffers.mat_indices->unmap();
    m.buffers.positions->unmap();
    if( m.has_normals )
        m.buffers.normals->unmap();
    if( m.has_texcoords)
        m.buffers.texcoords->unmap();

    m.tri_indices = nullptr;
    m.mat_indices = nullptr;
    m.positions   = nullptr;
    m.normals     = nullptr;
    m.texcoords   = nullptr;

    delete[] m.mat_params;
    m.mat_params = nullptr;
}
