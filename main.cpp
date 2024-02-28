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

#define VERSION "v0.0.1"

std::string ymd_date(){
    const std::chrono::time_point now(std::chrono::system_clock::now());
    const std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(now));

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year()) << '-'
       << std::setw(2) << static_cast<unsigned>(ymd.month()) << '-'
       << std::setw(2) << static_cast<unsigned>(ymd.day());

    return ss.str();
}

int path_check(const std::vector<std::string>& paths){
    for(const auto& item : paths){ // Check if program exist
        if(!std::filesystem::exists(item)){
            std::cerr << "Program path not found: (" << item << ").\n" << std::endl;
            return 0;
        }
    }

    return 1;
}


void create_save(const std::vector<std::string>& programPaths, const std::string& program, const std::string& exelocation){
    if(path_check(programPaths) == 1){ //Program paths verified
        
        analyzer configAnalyzer(programPaths, program, exelocation);
        if(configAnalyzer.is_identical_config()){
            // std::filesystem::rename(configAnalyzer.get_newest_backup_path(), );
        }
    }
}


int main(int argc, char* argv[]){
    
    std::cout << ymd_date() << std::endl;
    std::string x = ymd_date();
    std::cout << x << std::endl;
    return 0;
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
            programconfig jackett("Jackett", exePath.generic_string());
            std::vector<std::string> jackettPaths = jackett.get_config_paths();

            programconfig prowlarr("Prowlarr", exePath.generic_string());
            std::vector<std::string> prowlarrPaths = prowlarr.get_config_paths();

            programconfig qbittorrent("qBittorrent", exePath.generic_string());
            std::vector<std::string> qbittorrentPaths = qbittorrent.get_config_paths();
            
            // Create the archive if it does not exist yet
            if(!std::filesystem::exists(jackett.get_archive_path())){
                std::filesystem::create_directories(jackett.get_archive_path());
            }

            if(!std::filesystem::exists(prowlarr.get_archive_path())){
                std::filesystem::create_directories(prowlarr.get_archive_path());
            }

            if(!std::filesystem::exists(qbittorrent.get_archive_path())){
                std::filesystem::create_directories(qbittorrent.get_archive_path()); 
            }        
        }
    }

    
    return 0;
}