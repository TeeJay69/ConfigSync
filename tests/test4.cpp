#include <boost/filesystem/operations.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/detail/path_traits.hpp>

#include <iostream>

namespace fs = boost::filesystem;


int main(int argc, char* argv[])
{
    fs::path full_path(fs::initial_path<fs::path>());

    full_path = fs::system_complete(fs::path(argv[0]));

    std::cout << full_path << std::endl;

    //Without file extension
    std::cout << full_path.stem() << std::endl;
    std::cout << full_path.parent_path() << std::endl;
    //std::cout << fs::basename(full_path) << std::endl;

    std::cout << "argv[0]" << argv[0] << std::endl;
    return 0;
}