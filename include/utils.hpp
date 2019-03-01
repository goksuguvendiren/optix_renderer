//
// Created by Göksu Güvendiren on 2019-02-24.
//

#pragma once

#include <string>

namespace grpt
{
    namespace utils
    {
        std::string read_file(const std::string& path);
        std::string cuda_to_string(const std::string& cuda_file);
        std::string compile_cuda_source(const std::string& path, const std::string& source);
        void printUsageAndExit( const std::string& argv0, const std::string& sample_name );
    }
}