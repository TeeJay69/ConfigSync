#include <boost/filesystem/operations.hpp>

#include <boost/filesystem/path.hpp>

#include <iostream>

namespace fs = boost::filesystem;


int main(int argc, char* argv[])
{
    fs::path full_path( fs::initial_path<fs::path>() );

    full_path = fs::system_complete( fs::path( argv[0] ) );

    std::cout << full_path << std::endl;

    //Without file name
    std::cout << full_path.stem() << std::endl;
    //std::cout << fs::basename(full_path) << std::endl;

    return 0;
}