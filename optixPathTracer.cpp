
//-----------------------------------------------------------------------------
//
// optixMeshViewer: simple interactive mesh viewer
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

#include <sutil.h>
#include "cuda/commonStructs.h"
#include <Arcball.h>
#include <OptiXMesh.h>
#include <scene_parser.hpp>

#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <include/utils.hpp>

using namespace optix;

const char* const SAMPLE_NAME = "optixMeshViewer";

grpt::scene_parser parser;
auto scene = parser.LoadFromJson("../../mesh_input.json");

//------------------------------------------------------------------------------
//
// Globals
//
//------------------------------------------------------------------------------

//Context        context;
uint32_t       width  = 1024u;
uint32_t       height = 768u;
bool           use_pbo = true;
optix::Aabb    aabb;

// Camera state
float3         camera_up;
float3         camera_lookat;
float3         camera_eye;
Matrix4x4      camera_rotate;
sutil::Arcball arcball;

// Mouse state
int2           mouse_prev_pos;
int            mouse_button;


//------------------------------------------------------------------------------
//
// Forward decls
//
//------------------------------------------------------------------------------

struct UsageReportLogger;

Buffer getOutputBuffer();
void destroyContext();
void registerExitHandler();
void createContext( int usage_report_level, UsageReportLogger* logger );
void loadMesh( const std::string& filename );
void setupCamera();
void setupLights();
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
    return scene.ctx()[ "output_buffer" ]->getBuffer();
}


void destroyContext()
{
    if( scene.ctx() )
    {
        scene.ctx()->destroy();
        scene.ctx() = 0;
    }
}


struct UsageReportLogger
{
    void log( int lvl, const char* tag, const char* msg )
    {
        std::cout << "[" << lvl << "][" << std::left << std::setw( 12 ) << tag << "] " << msg;
    }
};

// Static callback
void usageReportCallback( int lvl, const char* tag, const char* msg, void* cbdata )
{
    // Route messages to a C++ object (the "logger"), as a real app might do.
    // We could have printed them directly in this simple case.

    UsageReportLogger* logger = reinterpret_cast<UsageReportLogger*>( cbdata );
    logger->log( lvl, tag, msg );
}

void registerExitHandler()
{
    // register shutdown handler
#ifdef _WIN32
    glutCloseFunc( destroyContext );  // this function is freeglut-only
#else
    atexit( destroyContext );
#endif
}


void createContext( int usage_report_level, UsageReportLogger* logger )
{
    // Set up context
    scene.ctx()->setRayTypeCount( 2 );
    scene.ctx()->setEntryPointCount( 1 );
    if( usage_report_level > 0 )
    {
        scene.ctx()->setUsageReportCallback( usageReportCallback, usage_report_level, logger );
    }

    scene.ctx()["radiance_ray_type"]->setUint( 0u );
    scene.ctx()["shadow_ray_type"  ]->setUint( 1u );
    scene.ctx()["scene_epsilon"    ]->setFloat( 1.e-4f );

    Buffer buffer = sutil::createOutputBuffer( scene.ctx(), RT_FORMAT_UNSIGNED_BYTE4, width, height, use_pbo );
    scene.ctx()["output_buffer"]->set( buffer );

    // Ray generation program
    const char *ptx = sutil::getPtxString( SAMPLE_NAME, "../src/ray_generators/pinhole_camera.cu" );
    Program ray_gen_program = scene.ctx()->createProgramFromPTXString( ptx, "pinhole_camera" );
    scene.ctx()->setRayGenerationProgram( 0, ray_gen_program );

    // Exception program
    Program exception_program = scene.ctx()->createProgramFromPTXString( ptx, "exception" );
    scene.ctx()->setExceptionProgram( 0, exception_program );
    scene.ctx()["bad_color"]->setFloat( 1.0f, 0.0f, 1.0f );

    // Miss program
    scene.ctx()->setMissProgram( 0, scene.ctx()->createProgramFromPTXString( sutil::getPtxString( SAMPLE_NAME, "../constantbg.cu" ), "miss" ) );
    scene.ctx()["bg_color"]->setFloat( 0.34f, 0.55f, 0.85f );
}


void loadMesh( const std::string& filename )
{
    OptiXMesh mesh;
    mesh.context = scene.ctx();
    loadMesh( filename, mesh );

    aabb.set( mesh.bbox_min, mesh.bbox_max );

    GeometryGroup geometry_group = scene.ctx()->createGeometryGroup();
    geometry_group->addChild( mesh.geom_instance );
    geometry_group->setAcceleration( scene.ctx()->createAcceleration( "Trbvh" ) );
    scene.ctx()[ "top_object"   ]->set( geometry_group );
    scene.ctx()[ "top_shadower" ]->set( geometry_group );
}

void setupCamera()
{
    auto& camera = scene.get_camera();

    camera_eye    = camera.eye();//make_float3( 278.0f, 273.0f, -900.0f );
    camera_lookat = camera.lookat();//make_float3( 278.0f, 273.0f,    0.0f );
    camera_up     = camera.up();//make_float3(   0.0f,   1.0f,    0.0f );

    camera_rotate  = camera.rotate();//Matrix4x4::identity();
}


void updateCamera()
{
    const float vfov = 35.0f;
    const float aspect_ratio = static_cast<float>(width) /
                               static_cast<float>(height);

    float3 camera_u, camera_v, camera_w;
    sutil::calculateCameraVariables(
            camera_eye, camera_lookat, camera_up, vfov, aspect_ratio,
            camera_u, camera_v, camera_w, true );

    const Matrix4x4 frame = Matrix4x4::fromBasis(
            normalize( camera_u ),
            normalize( camera_v ),
            normalize( -camera_w ),
            camera_lookat);
    const Matrix4x4 frame_inv = frame.inverse();
    // Apply camera rotation twice to match old SDK behavior
    const Matrix4x4 trans     = frame*camera_rotate*camera_rotate*frame_inv;

    camera_eye    = make_float3( trans*make_float4( camera_eye,    1.0f ) );
    camera_lookat = make_float3( trans*make_float4( camera_lookat, 1.0f ) );
    camera_up     = make_float3( trans*make_float4( camera_up,     0.0f ) );

    sutil::calculateCameraVariables(
            camera_eye, camera_lookat, camera_up, vfov, aspect_ratio,
            camera_u, camera_v, camera_w, true );

    camera_rotate = Matrix4x4::identity();

    scene.ctx()["eye"]->setFloat( camera_eye );
    scene.ctx()["U"  ]->setFloat( camera_u );
    scene.ctx()["V"  ]->setFloat( camera_v );
    scene.ctx()["W"  ]->setFloat( camera_w );
}


void glutInitialize( int* argc, char** argv )
{
    glutInit( argc, argv );
    glutInitDisplayMode( GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE );
    glutInitWindowSize( width, height );
    glutInitWindowPosition( 100, 100 );
    glutCreateWindow( SAMPLE_NAME );
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

    glViewport(0, 0, width, height);

    glutShowWindow();
    glutReshapeWindow( width, height);

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
    scene.ctx()->launch( 0, width, height );

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
            destroyContext();
            exit(0);
        }
        case( 's' ):
        {
            const std::string outputImage = std::string(SAMPLE_NAME) + ".ppm";
            std::cerr << "Saving current frame to '" << outputImage << "'\n";
            sutil::displayBufferPPM( outputImage.c_str(), getOutputBuffer() );
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
                         static_cast<float>( width );
        const float dy = static_cast<float>( y - mouse_prev_pos.y ) /
                         static_cast<float>( height );
        const float dmax = fabsf( dx ) > fabs( dy ) ? dx : dy;
        const float scale = fminf( dmax, 0.9f );
        camera_eye = camera_eye + (camera_lookat - camera_eye)*scale;
    }
    else if( mouse_button == GLUT_LEFT_BUTTON )
    {
        const float2 from = { static_cast<float>(mouse_prev_pos.x),
                              static_cast<float>(mouse_prev_pos.y) };
        const float2 to   = { static_cast<float>(x),
                              static_cast<float>(y) };

        const float2 a = { from.x / width, from.y / height };
        const float2 b = { to.x   / width, to.y   / height };

        camera_rotate = arcball.rotate( b, a );
    }

    mouse_prev_pos = make_int2( x, y );
}


void glutResize( int w, int h )
{
    if ( w == (int)width && h == (int)height ) return;

    width  = w;
    height = h;
    sutil::ensureMinimumSize(width, height);

    sutil::resizeBuffer( getOutputBuffer(), width, height );

    glViewport(0, 0, width, height);

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
    std::string mesh_file = std::string( sutil::samplesDir() ) + "/data/cow.obj";
    int usage_report_level = 0;
    for( int i=1; i<argc; ++i )
    {
        const std::string arg( argv[i] );

        if( arg == "-h" || arg == "--help" )
        {
            grpt::utils::printUsageAndExit( argv[0] , SAMPLE_NAME);
        }
        else if( arg == "-f" || arg == "--file"  )
        {
            if( i == argc-1 )
            {
                std::cerr << "Option '" << arg << "' requires additional argument.\n";
                grpt::utils::printUsageAndExit( argv[0], SAMPLE_NAME );
            }
            out_file = argv[++i];
        }
        else if( arg == "-n" || arg == "--nopbo"  )
        {
            use_pbo = false;
        }
        else if( arg == "-m" || arg == "--mesh" )
        {
            if( i == argc-1 )
            {
                std::cerr << "Option '" << argv[i] << "' requires additional argument.\n";
                grpt::utils::printUsageAndExit( argv[0] , SAMPLE_NAME);
            }
            mesh_file = argv[++i];
        }
        else if( arg == "-r" || arg == "--report" )
        {
            if( i == argc-1 )
            {
                std::cerr << "Option '" << argv[i] << "' requires additional argument.\n";
                grpt::utils::printUsageAndExit( argv[0], SAMPLE_NAME );
            }
            usage_report_level = atoi( argv[++i] );
        }
        else
        {
            std::cerr << "Unknown option '" << arg << "'\n";
            grpt::utils::printUsageAndExit( argv[0], SAMPLE_NAME );
        }
    }

    try
    {
//        glutInitialize( &argc, argv );

#ifndef __APPLE__
        glewInit();
#endif

        UsageReportLogger logger;
        createContext( usage_report_level, &logger );
        loadMesh( mesh_file );
        setupCamera();
        setupLights();

        scene.ctx()->validate();

//        if ( out_file.empty() )
//        {
//            glutRun();
//        }
//        else
        {
            updateCamera();
            scene.ctx()->launch( 0, width, height );
            sutil::displayBufferPPM( std::string{"../output_mash_viewer.ppm"}.c_str(), getOutputBuffer() );
            destroyContext();
        }
        return 0;
    }
    SUTIL_CATCH( scene.ctx()->get() )
}

