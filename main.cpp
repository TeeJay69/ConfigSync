#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <map>
#include <chrono>
#include <thread>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>
#include <boost\filesystem\operations.hpp>
#include <boost\filesystem\path.hpp>
#include "analyzer.hpp"
#include "programs.hpp"
#include "synchronizer.hpp"
#include "database.hpp"
#include "organizer.hpp"

#define VERSION "v0.0.1"


int main(int argc, char* argv[]){
    
    // Get location of exe
    boost::filesystem::path exePath(boost::filesystem::initial_path<boost::filesystem::path>());
    exePath = (boost::filesystem::system_complete(boost::filesystem::path(argv[0])));

    
    // Parse command line options:
    if(argv[1] == NULL){ // No params, display help
        std::cout << "ConfigSync (TJ-coreutils) " << VERSION << std::endl;
        std::cout << "Synchronize program configurations." << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    if(argv[2] == "version" || argv[2] == "--version"){ // Version
        std::cout << "ConfigSync " << VERSION << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    if(argv[2] == "help" || argv[2] == "--help"){ // Help message
        std::cout << "ConfigSync (TJ-coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::cout << "The Config-Synchronizer utility enables saving, restoring of programs configuration files.\n" << std::endl;
        std::cout << "usage: cfgs [<command>] [<options>]\n" << std::endl;
        std::cout << "Following commands are available:" << std::endl;
    }

    if(argv[2] == "sync" || argv[2] == "--sync" || argv[2] == "save" || argv[2] == "--save"){ // Create a save

        if(argv[3] == NULL){ // default behavior. Create a save of all supported, installed programs.
            
            // Get program paths
            
        }
    }

    
    return 0;
}