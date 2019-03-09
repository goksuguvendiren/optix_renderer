//
// Created by goksu on 2/6/19.
//

#include <utils.hpp>
#include <optix/optixu/optixpp.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <sampleConfig.h>
#include <dirent.h>
#include <sstream>
#include <nvrtc.h>
#include <include/utils.hpp>


bool dirExists( const char* path )
{
#if defined(_WIN32)
    DWORD attrib = GetFileAttributes( path );
    return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#else
    DIR* dir = opendir( path );
    if( dir == NULL )
        return false;

    closedir(dir);
    return true;
#endif
}

const char* samplesDir()
{
    static char s[512];

    // Allow for overrides.
    const char* dir = getenv( "OPTIX_SAMPLES_SDK_DIR" );
    if (dir) {
        strcpy(s, dir);
        return s;
    }

    // Return hardcoded path if it exists.
    if( dirExists( SAMPLES_DIR ) )
        return SAMPLES_DIR;

    // Last resort.
    return ".";
}
std::string grpt::utils::read_file(const std::string& path)
{
    std::ifstream cuda_file(path);
    if (!cuda_file.good()) throw "file not good";
    return std::string(dynamic_cast<std::stringstream const&>(std::stringstream() << cuda_file.rdbuf()).str());
}

std::string grpt::utils::compile_cuda_source(const std::string& path, const std::string& source)
{
// Create program
    nvrtcProgram prog = 0;
    nvrtcCreateProgram( &prog, source.c_str(), source.c_str(), 0, nullptr, nullptr);

    // Gather NVRTC options
    std::vector<const char *> options;

    std::string base_dir = std::string( samplesDir() );

    // Set sample dir as the primary include path
    std::string sample_dir;
//    if( sample_name )
//    {
        sample_dir = std::string( "-I" ) + base_dir + "/";
        options.push_back( sample_dir.c_str() );
//    }

    // Collect include dirs
    std::vector<std::string> include_dirs;
    const char *abs_dirs[] = { SAMPLES_ABSOLUTE_INCLUDE_DIRS };
    const char *rel_dirs[] = { SAMPLES_RELATIVE_INCLUDE_DIRS };

    const size_t n_abs_dirs = sizeof( abs_dirs ) / sizeof( abs_dirs[0] );
    for( size_t i = 0; i < n_abs_dirs; i++ )
        include_dirs.push_back(std::string( "-I" ) + abs_dirs[i]);
    const size_t n_rel_dirs = sizeof( rel_dirs ) / sizeof( rel_dirs[0] );
    for( size_t i = 0; i < n_rel_dirs; i++ )
        include_dirs.push_back(std::string( "-I" ) + base_dir + rel_dirs[i]);
    for( std::vector<std::string>::const_iterator it = include_dirs.begin(); it != include_dirs.end(); ++it )
        options.push_back( it->c_str() );

    // Collect NVRTC options
    const char *compiler_options[] = { CUDA_NVRTC_OPTIONS };
    const size_t n_compiler_options = sizeof( compiler_options ) / sizeof( compiler_options[0] );
    for( size_t i = 0; i < n_compiler_options - 1; i++ )
        options.push_back( compiler_options[i] );

    // JIT compile CU to PTX
    const nvrtcResult compileRes = nvrtcCompileProgram( prog, (int) options.size(), options.data() );

    // Retrieve log output
    size_t log_size = 0;
    nvrtcGetProgramLogSize( prog, &log_size );

    std::string nvrtc_log;
    nvrtc_log.resize( log_size );
    if( log_size > 1 )
    {
        nvrtcGetProgramLog( prog, &nvrtc_log[0] );
        std::cerr << nvrtc_log << '\n';
    }
    if( compileRes != NVRTC_SUCCESS )
        throw std::runtime_error( "NVRTC Compilation failed.\n" + nvrtc_log );

    // Retrieve PTX code
    size_t ptx_size = 0;
    nvrtcGetPTXSize( prog, &ptx_size ) ;

    std::string ptx;
    ptx.resize( ptx_size );
    nvrtcGetPTX( prog, &ptx[0]);

    // Cleanup
    nvrtcDestroyProgram( &prog );

    return ptx;
}

std::string grpt::utils::cuda_to_string(const std::string &cuda_file)
{
#ifdef CUDA_NVRTC_ENABLED
    std::string cuda_path = std::string(samplesDir()) + "/" + cuda_file;
    std::string cuda_source = grpt::utils::read_file(cuda_path);

    std::string ptx_compiled = grpt::utils::compile_cuda_source(cuda_path, cuda_source);
#else
    // getPtxStringFromFile();
#endif
    return ptx_compiled;
}

void grpt::utils::printUsageAndExit( const std::string& argv0, const std::string& sample_name )
{
    std::cerr << "\nUsage: " << argv0 << " [options]\n";
    std::cerr <<
              "App Options:\n"
              "  -h | --help               Print this usage message and exit.\n"
              "  -f | --file               Save single frame to file and exit.\n"
              "  -n | --nopbo              Disable GL interop for display buffer.\n"
              "App Keystrokes:\n"
              "  q  Quit\n"
              "  s  Save image to '" << sample_name << ".ppm'\n"
              << std::endl;

    exit(1);
}

std::string grpt::utils::getExtension(const std::string &filename)
{
    // Get the filename extension
    std::string::size_type extension_index = filename.find_last_of( "." );
    std::string ext =  extension_index != std::string::npos ?
                       filename.substr( extension_index+1 ) :
                       std::string();
    std::locale loc;
    for ( std::string::size_type i=0; i < ext.length(); ++i )
        ext[i] = std::tolower( ext[i], loc );

    return ext;
}

std::string grpt::utils::directoryOfFilePath(const std::string &filepath)
{
    size_t slash_pos, backslash_pos;
    slash_pos     = filepath.find_last_of( '/' );
    backslash_pos = filepath.find_last_of( '\\' );

    size_t break_pos;
    if( slash_pos == std::string::npos && backslash_pos == std::string::npos ) {
        return std::string();
    } else if ( slash_pos == std::string::npos ) {
        break_pos = backslash_pos;
    } else if ( backslash_pos == std::string::npos ) {
        break_pos = slash_pos;
    } else {
        break_pos = std::max(slash_pos, backslash_pos);
    }

    // Include the final slash
    return filepath.substr(0, break_pos + 1);
}
