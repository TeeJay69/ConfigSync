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

#ifdef DEBUG
#define DEBUG_MODE 1
#else 
#define DEBUG_MODE 0
#endif

#define VERSION "v0.1.0"


int path_check(const std::vector<std::string>& paths){
    for(const auto& item : paths){ // Check if program exist
        if(!std::filesystem::exists(item)){
            std::cerr << "Program path not found: (" << item << ").\n" << std::endl;
            return 0;
        }
    }

    return 1;
}


void create_save(const std::vector<std::string>& programPaths, const std::string& program, const std::string& pArchivePath, const std::string& exelocation){
    if(path_check(programPaths) == 1){ // Verify program paths        
        
        analyzer configAnalyzer(programPaths, program, exelocation);

        if(configAnalyzer.is_identical_config()){ // Config identical    
            std::cout << program + " config did not change, updating save date..." << std::endl;

            std::filesystem::path newestPath = configAnalyzer.get_newest_backup_path();
            std::filesystem::rename(newestPath, newestPath.parent_path().string() + "\\" + synchronizer::ymd_date()); // Update name of newest save dir

        }
        else{ // Config changed
            std::cout << program << " config changed.\nSynchronizing " << program << "..." << std::endl;
            synchronizer sync(programPaths, program, exelocation);

            if(sync.copy_config(pArchivePath, synchronizer::ymd_date()) == 0){
                std::cerr << "Error synchronizing " + program << "." << std::endl;
            }
        }
    }
    else{
        std::cerr << "Verification of " << program << " location failed." << std::endl;
    }
}


int main(int argc, char* argv[]){

    // Get location of exe
    boost::filesystem::path exePathBfs(boost::filesystem::initial_path<boost::filesystem::path>());
    exePathBfs = (boost::filesystem::system_complete(boost::filesystem::path(argv[0])));
    std::string exePath = exePathBfs.string();
    
    // Parse command line options:
    if(argv[1] == NULL){ // No params, display Copyright Notice
        std::cout << "ConfigSync (TJ-coreutils) " << VERSION << std::endl;
        std::cout << "Synchronize program configurations." << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber. All rights reserved." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    if(argv[2] == "version" || argv[2] == "--version"){ // Version @param
        std::cout << "ConfigSync " << VERSION << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    if(argv[2] == "help" || argv[2] == "--help"){ // Help message @param
        std::cout << "ConfigSync (TJ-coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::cout << "The Config-Synchronizer utility enables saving, restoring of programs configuration files.\n" << std::endl;
        std::cout << "usage: cfgs [<command>] [<options>]\n" << std::endl;
        std::cout << "Following commands are available:" << std::endl;
    }

    if(argv[2] == "sync" || argv[2] == "--sync" || argv[2] == "save" || argv[2] == "--save"){ // Create a save @param

        if(argv[3] == NULL){ // Default behavior. Create a save of all supported, installed programs. @param
            
            // Get program paths
            programconfig jackett("Jackett", exePath);
            std::vector<std::string> jackettPaths = jackett.get_config_paths();

            programconfig prowlarr("Prowlarr", exePath);
            std::vector<std::string> prowlarrPaths = prowlarr.get_config_paths();

            programconfig qbittorrent("qBittorrent", exePath);
            std::vector<std::string> qbittorrentPaths = qbittorrent.get_config_paths();
            

            // Create the archive @dir if it does not exist yet
            if(!std::filesystem::exists(jackett.get_archive_path())){
                std::filesystem::create_directories(jackett.get_archive_path());
            }

            if(!std::filesystem::exists(prowlarr.get_archive_path())){
                std::filesystem::create_directories(prowlarr.get_archive_path());
            }

            if(!std::filesystem::exists(qbittorrent.get_archive_path())){
                std::filesystem::create_directories(qbittorrent.get_archive_path()); 
            }        
            
            // @section Jackett
            create_save(jackettPaths, "Jackett", jackett.get_archive_path().string(), exePath);

            // @section Prowlarr
            create_save(prowlarrPaths, "Prowlarr", prowlarr.get_archive_path().string(), exePath);

            // @section qBittorrent
            create_save(qbittorrentPaths, "qBittorrent", qbittorrent.get_archive_path().string(), exePath);
        }
    }

    
    return 0;
}