////
//// Created by Göksu Güvendiren on 2019-03-01.
////
//
//#pragma once
//
//
//#include <optixu/optixpp_namespace.h>
//#include <optixu/optixu_math_stream_namespace.h>
//#include <Arcball.h>
//
//using namespace optix;
//
//const char* const SAMPLE_NAME = "optixPathTracer";
//
////------------------------------------------------------------------------------
////
//// Globals
////
////------------------------------------------------------------------------------
//
//Context        context = 0;
//uint32_t       width  = 512;
//uint32_t       height = 512;
//bool           use_pbo = true;
//bool           progressive = false;
//
//unsigned int   frame_number = 1;
//unsigned int   sqrt_num_samples = 10;
//int            rr_begin_depth = 1;
//Program        pgram_intersection = 0;
//Program        pgram_bounding_box = 0;
//
//// Camera state
//float3         camera_up;
//float3         camera_lookat;
//float3         camera_eye;
//Matrix4x4      camera_rotate;
//bool           camera_changed = true;
//sutil::Arcball arcball;
//
//// Mouse state
//int2           mouse_prev_pos;
//int            mouse_button;
//
//
////------------------------------------------------------------------------------
////
//// Forward decls
////
////------------------------------------------------------------------------------
//
//Buffer getOutputBuffer();
//void destroyContext();
//void registerExitHandler();
//void createContext();
//void loadGeometry();
//void setupCamera();
//void updateCamera();
//void glutInitialize( int* argc, char** argv );
//void glutRun();
//
//void glutDisplay();
//void glutKeyboardPress( unsigned char k, int x, int y );
//void glutMousePress( int button, int state, int x, int y );
//void glutMouseMotion( int x, int y);
//void glutResize( int w, int h );
