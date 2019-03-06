//-----------------------------------------------------------------------------
//
// optixPathTracer: simple interactive path tracer
//
//-----------------------------------------------------------------------------

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glew.h>
#  if defined( _WIN32 )
#    include <GL/wglew.h>
#    include <GL/freeglut.h>
#  else
#    include <GL/glut.h>
#  endif
#endif

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include "include/light_sources/point.hpp"
#include "include/light_sources/area.hpp"

#include "optixPathTracer.h"
#include <sutil.h>
#include <Arcball.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <chrono>
#include <experimental/filesystem>

#include <utils.hpp>
#include <camera.hpp>
#include <scene_parser.hpp>

using namespace optix;

//------------------------------------------------------------------------------
//
// Globals
//
//------------------------------------------------------------------------------

bool           use_pbo = true;
bool           progressive = false;

unsigned int   frame_number = 1;
//unsigned int   sqrt_num_samples = 10;
int            rr_begin_depth = 1;
Program        pgram_intersection = 0;
Program        pgram_bounding_box = 0;

// Camera state
float3         camera_up;
float3         camera_lookat;
float3         camera_eye;
Matrix4x4      camera_rotate;
bool           camera_changed = true;
sutil::Arcball arcball;

// Mouse state
int2           mouse_prev_pos;
int            mouse_button;

grpt::scene_parser parser;
auto scene = parser.LoadFromJson("../../input.json");

//------------------------------------------------------------------------------
//
// Forward decls 
//
//------------------------------------------------------------------------------

Buffer getOutputBuffer();
//void destroyContext();
void registerExitHandler();
void createContext();
void loadGeometry();
void setupCamera();
void updateCamera();
void glutInitialize( int* argc, char** argv );
void glutRun();

void glutDisplay();
void glutKeyboardPress( unsigned char k, int x, int y );
void glutMousePress( int button, int state, int x, int y );
void glutMouseMotion( int x, int y);
void glutResize( int w, int h );


//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------

Buffer getOutputBuffer()
{
//    return context[ "output_buffer" ]->getBuffer();
    return scene.ctx()[ "output_buffer" ]->getBuffer();
}


//void destroyContext()
//{
//    if( context )
//    {
//        context->destroy();
//        context = 0;
//    }
//}


void registerExitHandler()
{
    // register shutdown handler
#ifdef _WIN32
    glutCloseFunc( destroyContext );  // this function is freeglut-only
//#else
//    atexit( destroyContext );
#endif
}


void setMaterial(
        GeometryInstance& gi,
        Material material,
        const std::string& color_name,
        const float3& color)
{
    gi->addMaterial(material);
    gi[color_name]->setFloat(color);
}


GeometryInstance createParallelogram(
        const float3& anchor,
        const float3& offset1,
        const float3& offset2)
{
    Geometry parallelogram = scene.ctx()->createGeometry();
    parallelogram->setPrimitiveCount( 1u );
    parallelogram->setIntersectionProgram( pgram_intersection );
    parallelogram->setBoundingBoxProgram( pgram_bounding_box );

    float3 normal = normalize( cross( offset1, offset2 ) );
    float d = dot( normal, anchor );
    float4 plane = make_float4( normal, d );

    float3 v1 = offset1 / dot( offset1, offset1 );
    float3 v2 = offset2 / dot( offset2, offset2 );

    parallelogram["plane"]->setFloat( plane );
    parallelogram["anchor"]->setFloat( anchor );
    parallelogram["v1"]->setFloat( v1 );
    parallelogram["v2"]->setFloat( v2 );

    GeometryInstance gi = scene.ctx()->createGeometryInstance();
    gi->setGeometry(parallelogram);
    return gi;
}

void createContext()
{
//    context = Context::create();
//    context->setRayTypeCount( 2 );
//    context->setEntryPointCount( 1 );
//    context->setStackSize( 1800 );
//
//    context[ "scene_epsilon"                  ]->setFloat( 1.e-3f );
//    context[ "pathtrace_ray_type"             ]->setUint( 0u );
//    context[ "pathtrace_shadow_ray_type"      ]->setUint( 1u );
//    context[ "rr_begin_depth"                 ]->setUint( rr_begin_depth );
//
//    Buffer buffer = sutil::createOutputBuffer( context, RT_FORMAT_FLOAT4, scene.width, scene.height, use_pbo );
//    context["output_buffer"]->set( buffer );
//
//    // Setup programs
//    const char *ptx = sutil::getPtxString( sample_name.c_str(), "../optixPathTracer.cu" );
//    context->setRayGenerationProgram( 0, context->createProgramFromPTXString( ptx, "pathtrace_camera" ) );
//    context->setExceptionProgram( 0, context->createProgramFromPTXString( ptx, "exception" ) );
//    context->setMissProgram( 0, context->createProgramFromPTXString( ptx, "miss" ) );
//
//    context[ "sqrt_num_samples" ]->setUint( std::sqrt(scene.spp) );
//    context[ "bad_color"        ]->setFloat( scene.bad_color ); // Super magenta to make sure it doesn't get averaged out in the progressive rendering.
//    context[ "bg_color"         ]->setFloat( scene.bg_color );
}

void loadLight()
{
    // Light buffer
    grpt::area_light& light = scene.area_ls[0];

    Buffer light_buffer = scene.ctx()->createBuffer(RT_BUFFER_INPUT);
    light_buffer->setFormat(RT_FORMAT_USER);
    light_buffer->setElementSize(sizeof(grpt::area_light));
    light_buffer->setSize(1u);
    memcpy(light_buffer->map(), &light, sizeof(light));
    light_buffer->unmap();
    scene.ctx()["lights"]->setBuffer(light_buffer);

    optix::Buffer plight_buffer = scene.ctx()->createBuffer(RT_BUFFER_INPUT);
    plight_buffer->setFormat(RT_FORMAT_USER);
    plight_buffer->setElementSize(sizeof(grpt::point_light));
    plight_buffer->setSize(1u);

    memcpy(plight_buffer->map(), &scene.point_ls[0], sizeof(scene.point_ls[0]));
    plight_buffer->unmap();
    scene.ctx()["point_lights"]->setBuffer(plight_buffer);
}

void loadGeometry()
{
//    auto current_path = std::experimental::filesystem::current_path();
//    auto dir_home = current_path.parent_path();
//
//    auto lamb = dir_home.parent_path() / "src" / "shading_models" / "lambertian.cu";
//    assert(std::experimental::filesystem::exists(lamb));

    // Set up material
    Material diffuse = scene.ctx()->createMaterial();
    const char *ptx = sutil::getPtxString( scene.sample_name.c_str(), "../src/shading_models/lambertian.cu" );
    Program diffuse_ch = scene.ctx()->createProgramFromPTXString( ptx, "diffuse" );
    Program diffuse_ah = scene.ctx()->createProgramFromPTXString( ptx, "shadow" );
    diffuse->setClosestHitProgram( 0, diffuse_ch );
    diffuse->setAnyHitProgram( 1, diffuse_ah );

    Material diffuse_light = scene.ctx()->createMaterial();
    Program diffuse_em = scene.ctx()->createProgramFromPTXString( ptx, "diffuseEmitter" );
    diffuse_light->setClosestHitProgram( 0, diffuse_em );

    // Set up parallelogram programs
    ptx = sutil::getPtxString( scene.sample_name.c_str(), "../src/shapes/parallelogram.cu" );
    pgram_bounding_box = scene.ctx()->createProgramFromPTXString( ptx, "bounds" );
    pgram_intersection = scene.ctx()->createProgramFromPTXString( ptx, "intersect" );

    // create geometry instances
    std::vector<GeometryInstance> gis;

    const float3 white = make_float3( 0.8f, 0.8f, 0.8f );
    const float3 green = make_float3( 0.05f, 0.8f, 0.05f );
    const float3 red   = make_float3( 0.8f, 0.05f, 0.05f );
    const float3 light_em = make_float3( 15.0f, 15.0f, 5.0f );

    // Floor
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ),
                                        make_float3( 556.0f, 0.0f, 0.0f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Ceiling
    gis.push_back( createParallelogram( make_float3( 0.0f, 548.8f, 0.0f ),
                                        make_float3( 556.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Back wall
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 559.2f),
                                        make_float3( 0.0f, 548.8f, 0.0f),
                                        make_float3( 556.0f, 0.0f, 0.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Right wall
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 548.8f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", green);

    // Left wall
    gis.push_back( createParallelogram( make_float3( 556.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ),
                                        make_float3( 0.0f, 548.8f, 0.0f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", red);

    // Short block
    gis.push_back( createParallelogram( make_float3( 130.0f, 165.0f, 65.0f),
                                        make_float3( -48.0f, 0.0f, 160.0f),
                                        make_float3( 160.0f, 0.0f, 49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 290.0f, 0.0f, 114.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( -50.0f, 0.0f, 158.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 130.0f, 0.0f, 65.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( 160.0f, 0.0f, 49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 82.0f, 0.0f, 225.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( 48.0f, 0.0f, -160.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 240.0f, 0.0f, 272.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( -158.0f, 0.0f, -47.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Tall block
    gis.push_back( createParallelogram( make_float3( 423.0f, 330.0f, 247.0f),
                                        make_float3( -158.0f, 0.0f, 49.0f),
                                        make_float3( 49.0f, 0.0f, 159.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 423.0f, 0.0f, 247.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( 49.0f, 0.0f, 159.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 472.0f, 0.0f, 406.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( -158.0f, 0.0f, 50.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 314.0f, 0.0f, 456.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( -49.0f, 0.0f, -160.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 265.0f, 0.0f, 296.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( 158.0f, 0.0f, -49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Create shadow group (no light)
    GeometryGroup shadow_group = scene.ctx()->createGeometryGroup(gis.begin(), gis.end());
    shadow_group->setAcceleration( scene.ctx()->createAcceleration( "Trbvh" ) );
    scene.ctx()["top_shadower"]->set( shadow_group );

    // Light
    gis.push_back( createParallelogram( make_float3( 343.0f, 548.6f, 227.0f),
                                        make_float3( -130.0f, 0.0f, 0.0f),
                                        make_float3( 0.0f, 0.0f, 105.0f) ) );
    setMaterial(gis.back(), diffuse_light, "emission_color", light_em);

    // Create geometry group
    GeometryGroup geometry_group = scene.ctx()->createGeometryGroup(gis.begin(), gis.end());
    geometry_group->setAcceleration( scene.ctx()->createAcceleration( "Trbvh" ) );
    scene.ctx()["top_object"]->set( geometry_group );
}

grpt::camera camera;

void setupCamera()
{
    camera_eye    = camera.eye();//make_float3( 278.0f, 273.0f, -900.0f );
    camera_lookat = camera.lookat();//make_float3( 278.0f, 273.0f,    0.0f );
    camera_up     = camera.up();//make_float3(   0.0f,   1.0f,    0.0f );

    camera_rotate  = camera.rotate();//Matrix4x4::identity();
}


void updateCamera()
{
    const float fov  = 35.0f;
    const float aspect_ratio = static_cast<float>(scene.width) / static_cast<float>(scene.height);
    
    float3 camera_u, camera_v, camera_w;
//    sutil::calculateCameraVariables(
//            camera_eye, camera_lookat, camera_up, fov, aspect_ratio,
//            camera_u, camera_v, camera_w, /*fov_is_vertical*/ true );
//
//    const Matrix4x4 frame = Matrix4x4::fromBasis(
//            normalize( camera_u ),
//            normalize( camera_v ),
//            normalize( -camera_w ),
//            camera_lookat);
//    const Matrix4x4 frame_inv = frame.inverse();
//    // Apply camera rotation twice to match old SDK behavior
//    const Matrix4x4 trans     = frame*camera_rotate*camera_rotate*frame_inv;
//
//    camera_eye    = make_float3( trans*make_float4( camera_eye,    1.0f ) );
//    camera_lookat = make_float3( trans*make_float4( camera_lookat, 1.0f ) );
//    camera_up     = make_float3( trans*make_float4( camera_up,     0.0f ) );
//
//    sutil::calculateCameraVariables(
//            camera_eye, camera_lookat, camera_up, fov, aspect_ratio,
//            camera_u, camera_v, camera_w, true );
//
//    camera_rotate = Matrix4x4::identity();
//

    std::tie(camera_u, camera_v, camera_w) = camera.Update(camera_eye, camera_lookat, camera_up, camera_rotate);

    if( camera_changed ) // reset accumulation
        frame_number = 1;
    camera_changed = false;

    scene.ctx()[ "frame_number" ]->setUint( frame_number++ );
    scene.ctx()[ "eye"]->setFloat( camera_eye );
    scene.ctx()[ "U"  ]->setFloat( camera_u );
    scene.ctx()[ "V"  ]->setFloat( camera_v );
    scene.ctx()[ "W"  ]->setFloat( camera_w );
}


void glutInitialize( int* argc, char** argv )
{
    glutInit( argc, argv );
    glutInitDisplayMode( GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE );
    glutInitWindowSize( scene.width, scene.height );
    glutInitWindowPosition( 100, 100 );
    glutCreateWindow( scene.sample_name.c_str() );
    glutHideWindow();                                                              
}


void glutRun()
{
    // Initialize GL state                                                            
    glMatrixMode(GL_PROJECTION);                                                   
    glLoadIdentity();                                                              
    glOrtho(0, 1, 0, 1, -1, 1 );                                                   

    glMatrixMode(GL_MODELVIEW);                                                    
    glLoadIdentity();                                                              

    glViewport(0, 0, scene.width, scene.height);

    glutShowWindow();                                                              
    glutReshapeWindow( scene.width, scene.height);

    // register glut callbacks
    glutDisplayFunc( glutDisplay );
    glutIdleFunc( glutDisplay );
    glutReshapeFunc( glutResize );
    glutKeyboardFunc( glutKeyboardPress );
    glutMouseFunc( glutMousePress );
    glutMotionFunc( glutMouseMotion );

    registerExitHandler();

    glutMainLoop();
}


//------------------------------------------------------------------------------
//
//  GLUT callbacks
//
//------------------------------------------------------------------------------

void glutDisplay()
{
    updateCamera();
    scene.ctx()->launch( 0, scene.width, scene.height );

    sutil::displayBufferGL( getOutputBuffer() );

    {
      static unsigned frame_count = 0;
      sutil::displayFps( frame_count++ );
    }

    glutSwapBuffers();
}


void glutKeyboardPress( unsigned char k, int x, int y )
{

    switch( k )
    {
        case( 'q' ):
        case( 27 ): // ESC
        {
//            destroyContext();
            exit(0);
        }
        case( 's' ):
        {
            const std::string outputImage = std::string(scene.sample_name.c_str()) + ".ppm";
            std::cerr << "Saving current frame to '" << outputImage << "'\n";
            sutil::displayBufferPPM( outputImage.c_str(), getOutputBuffer(), false );
            break;
        }
    }
}


void glutMousePress( int button, int state, int x, int y )
{
    if( state == GLUT_DOWN )
    {
        mouse_button = button;
        mouse_prev_pos = make_int2( x, y );
    }
    else
    {
        // nothing
    }
}


void glutMouseMotion( int x, int y)
{
    if( mouse_button == GLUT_RIGHT_BUTTON )
    {
        const float dx = static_cast<float>( x - mouse_prev_pos.x ) /
                         static_cast<float>( scene.width );
        const float dy = static_cast<float>( y - mouse_prev_pos.y ) /
                         static_cast<float>( scene.height );
        const float dmax = fabsf( dx ) > fabs( dy ) ? dx : dy;
        const float scale = std::min<float>( dmax, 0.9f );
        camera_eye = camera_eye + (camera_lookat - camera_eye)*scale;
        camera_changed = true;
    }
    else if( mouse_button == GLUT_LEFT_BUTTON )
    {
        const float2 from = { static_cast<float>(mouse_prev_pos.x),
                              static_cast<float>(mouse_prev_pos.y) };
        const float2 to   = { static_cast<float>(x),
                              static_cast<float>(y) };

        const float2 a = { from.x / scene.width, from.y / scene.height };
        const float2 b = { to.x   / scene.width, to.y   / scene.height };

        camera_rotate = arcball.rotate( b, a );
        camera_changed = true;
    }

    mouse_prev_pos = make_int2( x, y );
}


void glutResize( int w, int h )
{
    if ( w == (int)scene.width && h == (int)scene.height ) return;

    camera_changed = true;

    scene.width  = w;
    scene.height = h;
    sutil::ensureMinimumSize(scene.width, scene.height);

    sutil::resizeBuffer( getOutputBuffer(), scene.width, scene.height );

    glViewport(0, 0, scene.width, scene.height);

    glutPostRedisplay();
}


//------------------------------------------------------------------------------
//
// Main
//
//------------------------------------------------------------------------------



int main( int argc, char** argv )
{
    std::string out_file;
    for( int i=1; i<argc; ++i )
    {
        const std::string arg( argv[i] );

        if( arg == "-h" || arg == "--help" )
        {
            grpt::utils::printUsageAndExit( argv[0], scene.sample_name.c_str());
        }
        else if( arg == "-f" || arg == "--file"  )
        {
            if( i == argc-1 )
            {
                std::cerr << "Option '" << arg << "' requires additional argument.\n";
                grpt::utils::printUsageAndExit( argv[0], scene.sample_name.c_str() );
            }
            out_file = argv[++i];
        }
        else if( arg == "-n" || arg == "--nopbo"  )
        {
            use_pbo = false;
        }
        else if( arg == "-p" || arg == "--progressive"  )
        {
            progressive = true;
        }
        else
        {
            std::cerr << "Unknown option '" << arg << "'\n";
            grpt::utils::printUsageAndExit( argv[0], scene.sample_name.c_str() );
        }
    }

    try
    {
//        glutInitialize( &argc, argv );

#ifndef __APPLE__
        glewInit();
#endif

        createContext();
        setupCamera();
        loadLight();
        loadGeometry();

        scene.ctx()->validate();

        if ( progressive )
        {
            glutRun();
        }
        else
        {
            auto begin = std::chrono::system_clock::now();
            updateCamera();
            scene.ctx()->launch( 0, scene.width, scene.height );
            auto end = std::chrono::system_clock::now();

            std::cout << "Rendering " << scene.spp << " samples per pixel took : " <<
                          std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms.\n";

            sutil::displayBufferPPM( std::string{"../output" + std::to_string(scene.spp) + ".ppm"}.c_str(), getOutputBuffer(), false );
//            destroyContext();
            std::cout << "done\n";
        }

        return 0;
    }
    SUTIL_CATCH( scene.ctx()->get() )
}

