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
            // std::cerr << "Program path not found: (" << item << ").\n" << std::endl;
            return 0;
        }
    }

    return 1;
}


int create_save(const std::vector<std::string>& programPaths, const std::string& program, const std::string& pArchivePath, const std::string& exelocation){
    if(path_check(programPaths) == 1){ // Verify program paths        
        
        analyzer configAnalyzer(programPaths, program, exelocation);

        if(configAnalyzer.is_identical_config()){ // Config identical    
            std::cout << program + " config did not change, updating save date..." << std::endl;

            std::filesystem::path newestPath = configAnalyzer.get_newest_backup_path();
            std::filesystem::rename(newestPath, newestPath.parent_path().string() + "\\" + synchronizer::ymd_date()); // Update name of newest save dir

            return 1;
        }
        else{ // Config changed
            std::cout << program << " config changed.\nSynchronizing " << program << "..." << std::endl;

            synchronizer sync(programPaths, program, exelocation); // initialize class

            if(sync.copy_config(pArchivePath, synchronizer::ymd_date()) != 1){ // sync config   
                std::cerr << "Error synchronizing " + program << "." << std::endl; // copy_config returned 0
                return 0;
            }
        }
    }
    else{
        std::cerr << "Verification of " << program << " location failed." << std::endl;
        return 0;
    }

    return 1;
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


    if(argv[2] == "sync" || argv[2] == "--sync" || argv[2] == "save" || argv[2] == "--save"){ // Sync @param

        if(argv[3] == NULL){ // Default behavior. Create a save of all supported, installed programs. No subparam provided
            
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
            
            std::vector<std::string> syncedList; // For sync message

            // Sync Jackett
            int jackettSync = 0;
            if(create_save(jackettPaths, "Jackett", jackett.get_archive_path().string(), exePath) == 1){
                jackettSync = 1;
                syncedList.push_back("Jackett");
            }

            // Sync Prowlarr
            int prowlarrSync = 0;
            if(create_save(prowlarrPaths, "Prowlarr", prowlarr.get_archive_path().string(), exePath) == 1){
                prowlarrSync = 1;
                syncedList.push_back("Prowlarr");
            }

            // Sync qBittorrent
            int qbittorrentSync = 0;
            if(create_save(qbittorrentPaths, "qBittorrent", qbittorrent.get_archive_path().string(), exePath) == 1){
                qbittorrentSync = 1;
                syncedList.push_back("qBittorrent");
            };


            // Info
            std::cout << ANSI_COLOR_GREEN << "Synchronization finished." << ANSI_COLOR_RESET << std::endl;
            std::cout << "Synced Programs:" << std::endl;

            int i = 1;
            for(const auto& item : syncedList){
                std::cout << ANSI_COLOR_GREEN << "      " << i << "." << item << ANSI_COLOR_RESET << std::endl;
                i++;
            }
        }

        else if(argv[3] == "jackett" || argv[3] == "Jackett"){ // Jackett sync @subparam
            // Get program path
            programconfig jackett("Jackett", exePath);
            std::vector<std::string> pPaths = jackett.get_config_paths();

            // Create archive if it doesnt exist
            if(!std::filesystem::exists(jackett.get_archive_path())){
                std::filesystem::create_directories(jackett.get_archive_path());
            }


            if(create_save(pPaths, "Jackett", jackett.get_archive_path().string(), exePath) == 1){ // save config
                std::cout << ANSI_COLOR_GREEN << "Synchronization of Jackett finished." << ANSI_COLOR_RESET << std::endl;
            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Jackett failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(argv[3] == "prowlarr" || argv[3] == "Prowlarr"){ // Prowlarr sync @subparam
            // Get program paths
            programconfig prowlarr("Prowlarr", exePath);
            std::vector<std::string> pPaths = prowlarr.get_config_paths();

            // Create archive if it doesnt exists
            if(!std::filesystem::create_directories(prowlarr.get_archive_path())){
                std::filesystem::create_directories(prowlarr.get_archive_path());
            }

            if(create_save(pPaths, "Prowlarr", prowlarr.get_archive_path().string(), exePath) == 1){
                std::cout << ANSI_COLOR_GREEN << "Synchronization of Prowlarr finished." << ANSI_COLOR_RESET << std::endl;

            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Prowlarr failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(argv[3] == "qBittorrent" || argv[3] == "qbittorrent"){  // qBittorrent sync @subparam
            // Get program paths
            programconfig qbittorrent("qBittorrent", exePath);
            std::vector<std::string> pPaths = qbittorrent.get_config_paths();

            // Create archive if it doesnt exists
            if(!std::filesystem::create_directories(qbittorrent.get_archive_path())){
                std::filesystem::create_directories(qbittorrent.get_archive_path());
            }

            if(create_save(pPaths, "qBittorrent", qbittorrent.get_archive_path().string(), exePath) == 1){
                std::cout << ANSI_COLOR_GREEN << "Synchronization of qBittorrent finished." << ANSI_COLOR_RESET << std::endl;

            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of qBittorrent failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else{ // Invalid parameter provided
            std::cerr << "'" << argv[3] << "' is not a configsync command. See 'cfgs --help'." << std::endl;
        }
    }


    else if(argv[2] == "..."){ // ... @param
        // TODO next parameter
    }

    
    return 0;
}