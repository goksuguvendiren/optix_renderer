//
// Created by Göksu Güvendiren on 2019-03-01.
//

#include <glut_handler.hpp>
#include <sutil.h>

namespace grpt
{
    void glutDisplay()
    {
        updateCamera();
        context->launch( 0, width, height );

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
                             static_cast<float>( width );
            const float dy = static_cast<float>( y - mouse_prev_pos.y ) /
                             static_cast<float>( height );
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

            const float2 a = { from.x / width, from.y / height };
            const float2 b = { to.x   / width, to.y   / height };

            camera_rotate = arcball.rotate( b, a );
            camera_changed = true;
        }

        mouse_prev_pos = make_int2( x, y );
    }


    void glutResize( int w, int h )
    {
        if ( w == (int)width && h == (int)height ) return;

        camera_changed = true;

        width  = w;
        height = h;
        sutil::ensureMinimumSize(width, height);

        sutil::resizeBuffer( getOutputBuffer(), width, height );

        glViewport(0, 0, width, height);

        glutPostRedisplay();
    }
}