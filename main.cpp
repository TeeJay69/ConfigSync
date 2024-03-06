#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <map>
#include <chrono>
#include <thread>
#include <set>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>
#include <boost\filesystem\operations.hpp>
#include <boost\filesystem\path.hpp>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\json_parser.hpp>
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

#define SETTINGS_ID 1
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
        
        if(configAnalyzer.is_archive_empty() == 1){ // First ever save.
            std::cout << "First synchronization of " << program << "." << std::endl;
            synchronizer sync(programPaths, program, exelocation); // initialize class

            if(sync.copy_config(pArchivePath, synchronizer::ymd_date()) != 1){ // sync config   
                std::cerr << "Error synchronizing " + program << "." << std::endl; // copy_config returned 0
                return 0;
            }
        }
        else{
            if(configAnalyzer.is_identical_config()){ // Compare last save to current config
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
    

    // Settings @section
    boost::property_tree::ptree pt;
    
    if(std::filesystem::exists("config.json") == false){ // Create default settings
        
        pt.put("settingsID", SETTINGS_ID); // Settings identifier for updates

        pt.put("RECYCLELIMIT", 3);
        pt.put("SAVELIMIT", 3);
        
        boost::property_tree::write_json("config.json", pt);

        

    }
    else{ // Settings file exists
        boost::property_tree::read_json("config.json", pt); // read config
        int settingsID = pt.get<int>("settingsID");


        if(settingsID == SETTINGS_ID){ // Matching settingsID
            // Do nothing, pt contains correct config
        }
        else{ // Differing settingsID
            //! merge config.
            // Write this section when you create a version that made changes to the config file.
            // Create the new config file after merging.
        }
    }

    
    
    // Parse command line options:
    if(argv[1] == NULL){ // No params, display Copyright Notice
        std::cout << "ConfigSync (TJ-coreutils) " << VERSION << std::endl;
        std::cout << "Synchronize program configurations." << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber. All rights reserved." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    if(std::string(argv[1]) == "version" || std::string(argv[1]) == "--version"){ // Version @param
        std::cout << "ConfigSync " << VERSION << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    if(std::string(argv[1]) == "help" || std::string(argv[1]) == "--help"){ // Help message @param
        std::cout << "ConfigSync (TJ-coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::cout << "The Config-Synchronizer utility enables saving, restoring of programs configuration files.\n" << std::endl;
        std::cout << "usage: cfgs [<command>] [<options>]\n" << std::endl;
        std::cout << "Following commands are available:" << std::endl;
    }


    if(std::string(argv[1]) == "sync" || std::string(argv[1]) == "--sync" || std::string(argv[1]) == "save" || std::string(argv[1]) == "--save"){ // Sync @param

        if(argv[2] == NULL){ // Default behavior. Create a save of all supported, installed programs. No subparam provided
            
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

            
            organizer janitor; // Initialize class

            // Sync Jackett
            int jackettSync = 0;
            if(create_save(jackettPaths, "Jackett", jackett.get_archive_path().string(), exePath) == 1){
                jackettSync = 1;
                syncedList.push_back("Jackett");

                // Limit number of saves
                janitor.limit_enforcer_configarchive(pt.get<int>("SAVELIMIT"), jackett.get_archive_path().string()); // Cleanup
            }

            // Sync Prowlarr
            int prowlarrSync = 0;
            if(create_save(prowlarrPaths, "Prowlarr", prowlarr.get_archive_path().string(), exePath) == 1){
                prowlarrSync = 1;
                syncedList.push_back("Prowlarr");

                // Limit number of saves
                janitor.limit_enforcer_configarchive(pt.get<int>("SAVELIMIT"), prowlarr.get_archive_path().string()); // Cleanup
            }

            // Sync qBittorrent
            int qbittorrentSync = 0;
            if(create_save(qbittorrentPaths, "qBittorrent", qbittorrent.get_archive_path().string(), exePath) == 1){
                qbittorrentSync = 1;
                syncedList.push_back("qBittorrent");

                // Limit number of saves
                janitor.limit_enforcer_configarchive(pt.get<int>("SAVELIMIT"), qbittorrent.get_archive_path().string()); // Cleanup
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

        else if(std::string(argv[2]) == "jackett" || std::string(argv[2]) == "Jackett"){ // Jackett sync @subparam
            // Get program path
            programconfig jackett("Jackett", exePath);
            std::vector<std::string> pPaths = jackett.get_config_paths();

            // Create archive if it doesnt exist
            if(!std::filesystem::exists(jackett.get_archive_path())){
                std::filesystem::create_directories(jackett.get_archive_path());
            }
            
            
            if(create_save(pPaths, "Jackett", jackett.get_archive_path().string(), exePath) == 1){ // save config
                // Limit num of saves
                organizer janitor;
                janitor.limit_enforcer_configarchive(pt.get<int>("SAVELIMIT"), jackett.get_archive_path().string()); // Cleanup

                std::cout << ANSI_COLOR_GREEN << "Synchronization of Jackett finished." << ANSI_COLOR_RESET << std::endl;
            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Jackett failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(std::string(argv[2]) == "prowlarr" || std::string(argv[2]) == "Prowlarr"){ // Prowlarr sync @subparam
            // Get program paths
            programconfig prowlarr("Prowlarr", exePath);
            std::vector<std::string> pPaths = prowlarr.get_config_paths();

            // Create archive if it doesnt exists
            if(!std::filesystem::create_directories(prowlarr.get_archive_path())){
                std::filesystem::create_directories(prowlarr.get_archive_path());
            }

            if(create_save(pPaths, "Prowlarr", prowlarr.get_archive_path().string(), exePath) == 1){
                // Limit num of saves
                organizer janitor;
                janitor.limit_enforcer_configarchive(pt.get<int>("SAVELIMIT"), prowlarr.get_archive_path().string()); // Cleanup

                std::cout << ANSI_COLOR_GREEN << "Synchronization of Prowlarr finished." << ANSI_COLOR_RESET << std::endl;

            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Prowlarr failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){  // qBittorrent sync @subparam
            // Get program paths
            programconfig qbittorrent("qBittorrent", exePath);
            std::vector<std::string> pPaths = qbittorrent.get_config_paths();

            // Create archive if it doesnt exists
            if(!std::filesystem::create_directories(qbittorrent.get_archive_path())){
                std::filesystem::create_directories(qbittorrent.get_archive_path());
            }

            if(create_save(pPaths, "qBittorrent", qbittorrent.get_archive_path().string(), exePath) == 1){
                // Limit num of saves
                organizer janitor;
                janitor.limit_enforcer_configarchive(pt.get<int>("SAVELIMIT"), qbittorrent.get_archive_path().string()); // Cleanup

                std::cout << ANSI_COLOR_GREEN << "Synchronization of qBittorrent finished." << ANSI_COLOR_RESET << std::endl;

            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of qBittorrent failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else{ // Invalid subparam provided
            std::cerr << "'" << argv[2] << "' is not a configsync command. See 'cfgs --help'." << std::endl;
        }
    }


    else if(std::string(argv[1]) == "restore" || std::string(argv[1]) == "--restore"){ // Restore @param

        if(argv[2] == NULL){ // No program provided. Display advice. No subparam provided.
            std::cout << "Fatal: Missing program or restore argument." << std::endl;
            std::cout << "For usage see 'cfgs --help'" << std::endl;
        }

        
        else if(std::string(argv[2]) == "all" || std::string(argv[2]) == "--all"){ // '--all' @subparam
            std::cout << ANSI_COLOR_YELLOW << "Restoring all supported programs with latest save..." << ANSI_COLOR_RESET << std::endl;
            
            std::vector<std::string> failList;

            // @section Jackett:
            programconfig jackett("Jackett", exePath); // Initialize class
            std::vector<std::string> pPaths = jackett.get_config_paths(); // Get program paths
            
            analyzer anlyJackett(pPaths, "Jackett", exePath); // Initialize class
            synchronizer syncJackett(pPaths, "Jackett", exePath); // Initialize class

            int jackettState = 0;
            if(anlyJackett.is_archive_empty() == 1){ // Archive is empty
                std::cout << "Skipping Jackett, not synced..." << std::endl;
                failList.push_back("Jackett");
            }
            else{ // Archive not empty
                if(syncJackett.restore_config(anlyJackett.get_newest_backup_path(), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from newest save
                    std::cout << ANSI_COLOR_GREEN << "Jackett rollback successfull!" << ANSI_COLOR_RESET << std::endl;
                    jackettState = 1;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "Jackett rollback failed." << ANSI_COLOR_RESET << std::endl;
                    failList.push_back("Jackett");
                }
            }

            
            // @section Prowlarr
            programconfig prowlarr("Prowlarr", exePath); // Initialize class
            std::vector<std::string> pPaths = prowlarr.get_config_paths(); // Get program paths
            
            analyzer anlyProw(pPaths, "Prowlarr", exePath); // Initialize class
            synchronizer syncProw(pPaths, "Prowlarr", exePath); // Initialize class

            int prowlarrState = 0;
            if(anlyProw.is_archive_empty() == 1){ // Archive empty
                std::cout << "Skipping Prowlarr, not synced..." << std::endl;
                failList.push_back("Prowlarr");
            }
            else{ // Archive not empty
                if(syncProw.restore_config(anlyProw.get_newest_backup_path(), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from newest save
                    std::cout << ANSI_COLOR_GREEN << "Prowlarr rollback successfull!" << ANSI_COLOR_RESET << std::endl;
                    prowlarrState = 1;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "Prowlarr rollback failed." << ANSI_COLOR_RESET << std::endl;
                    failList.push_back("Prowlarr");
                }
            }


            // @section qBittorrent
            programconfig qbittorrent("qBittorrent", exePath); // Initialize class
            std::vector<std::string> pPaths = qbittorrent.get_config_paths(); // Get program paths
            
            analyzer anlyQbit(pPaths, "qBittorrent", exePath); // Initialize class
            synchronizer syncQbit(pPaths, "qBittorrent", exePath); // Initialize class
            
            int qbitState = 0;
            if(anlyQbit.is_archive_empty() == 1){ // Archive empty
                std::cout << "Skipping qBittorrent, not synced..." << std::endl;
                failList.push_back("qBittorrent");
            }
            else{ // Archive not empty
                if(syncQbit.restore_config(anlyQbit.get_newest_backup_path(), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from newest save
                    std::cout << ANSI_COLOR_GREEN << "qBittorrent rollback successfull!" << ANSI_COLOR_RESET << std::endl;
                    qbitState = 1;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "qBittorrent rollback failed." << ANSI_COLOR_RESET << std::endl;
                    failList.push_back("qBittorrent");
                }
            }

            
            // User info
            if(jackettState == 1 && prowlarrState == 1 && qbitState == 1){ // All success
                std::cout << ANSI_COLOR_GREEN << "Rollback complete!" << ANSI_COLOR_RESET << std::endl; 
            }
            else if(jackettState == 1 || prowlarrState == 1 || qbitState == 1){ // Partial success
                std::cout << ANSI_COLOR_YELLOW << "Rollback complete!." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_RED << "Failed to restore:" << ANSI_COLOR_RESET << std::endl;
                
                int i = 1;
                for(const auto& app : failList){ // Display failed rollbacks
                    std::cout << ANSI_COLOR_RED << i << ".  " << app << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
            }
        }


        else if(std::string(argv[2]) == "Jackett" || std::string(argv[2]) == "jackett"){ // Jackett @subparam
            
            programconfig jackett("Jackett", exePath); // Initialize class
            std::vector<std::string> pPaths = jackett.get_config_paths(); // Get program paths
            
            analyzer anly(pPaths, "Jackett", exePath); // Initialize class

            std::vector<std::string> jackettSaves;
            anly.get_all_saves(jackettSaves);


            if(argv[3] == NULL){ // default behavior. Use latest save. No 2subparam provided

                if(anly.is_archive_empty() == 1){ // Archive empty
                    std::cout << "Archive does not contain previous saves of Jackett." << std::endl;
                    std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
                }
                else{ // Archive not empty
                    synchronizer sync(pPaths, "Jackett", exePath); // Initialize class

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }

                }
            }


            else if(std::find(jackettSaves.begin(), jackettSaves.end(), std::string(argv[3])) != jackettSaves.end()){ // Specific save date @2subparam. Check if date is valid.

                synchronizer sync(pPaths, "Jackett", exePath); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from user defined save date<
                    std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                }
            }

            else{ // Invalid 2subparameter (not null + not valid)
                std::cerr << "Fatal: '" << argv[3] << "' is not a valid date." << std::endl;
                std::cout << "To view all save dates use 'cfgs --show jackett'" << std::endl;
            }
        }


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr @subparam
            programconfig prowlarr("Prowlarr", exePath); // Initialize class
            std::vector<std::string> pPaths = prowlarr.get_config_paths(); // Get program paths
            
            analyzer anly(pPaths, "Prowlarr", exePath); // Initialize class

            std::vector<std::string> prowlarrSaves;
            anly.get_all_saves(prowlarrSaves);


            if(argv[3] == NULL){ // default behavior. Use latest save. No 2subparam provided

                if(anly.is_archive_empty() == 1){ // Archive empty
                    std::cout << "Archive does not contain previous saves of Prowlarr." << std::endl;
                    std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
                }
                else{ // Archive not empty
                    synchronizer sync(pPaths, "Prowlarr", exePath); // Initialize class

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }


            else if(std::find(prowlarrSaves.begin(), prowlarrSaves.end(), std::string(argv[3])) != prowlarrSaves.end()){ // Specific save date @2subparam. Check if date is valid.

                synchronizer sync(pPaths, "Prowlarr", exePath); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from user defined save date
                    std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                }
            }

            else{ // Invalid 2subparameter (not null + not valid)
                std::cerr << "Fatal: '" << argv[3] << "' is not a valid date." << std::endl;
                std::cout << "To view all save dates use 'cfgs --show prowlarr'" << std::endl;
            }
        }
        

        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent @subparam
            programconfig qbittorrent("qBittorrent", exePath); // Initialize class
            std::vector<std::string> pPaths = qbittorrent.get_config_paths(); // Get program paths
            
            analyzer anly(pPaths, "qBittorrent", exePath); // Initialize class

            std::vector<std::string> qbittorrentSaves;
            anly.get_all_saves(qbittorrentSaves);


            if(argv[3] == NULL){ // default behavior. Use latest save. No 2subparam provided

                if(anly.is_archive_empty() == 1){ // Archive empty
                    std::cout << "Archive does not contain previous saves of Jackett." << std::endl;
                    std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
                }
                else{ // Archive not empty
                    synchronizer sync(pPaths, "qBittorrent", exePath); // Initialize class

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }


            else if(std::find(qbittorrentSaves.begin(), qbittorrentSaves.end(), std::string(argv[3])) != qbittorrentSaves.end()){ // Specific save date @2subparam. Check if date is valid.

                synchronizer sync(pPaths, "qBittorrent", exePath); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("RECYCLELIMIT")) == 1){ // Restore from user defined save date
                    std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                }
            }

            else{ // Invalid 2subparameter (not null + not valid)
                std::cerr << "Fatal: '" << argv[3] << "' is not a valid date." << std::endl;
                std::cout << "To view all save dates use 'cfgs --show qbittorrent'" << std::endl;
            }
        }
    }


    else if(std::string(argv[1]) == "status" || std::string(argv[1]) == "--status"){ // 'status' @param

        if(argv[2] == NULL || std::string(argv[2]) == "--all"){ // default: all. @subparam

            std::set<std::string> neverSaveList;
            std::set<std::string> outofSyncList;
            std::set<std::string> inSyncList;


            for(const auto& app : programconfig::get_support_list()){ // Fetch status

                programconfig pcfg(app, exePath); // Init class
                analyzer anly(pcfg.get_config_paths(), app, exePath); // Init class

                if(anly.is_archive_empty() == 1){ // Check for previous save
                    neverSaveList.insert(app);
                }
                else{ // Save exists
                    
                    if(anly.is_identical_config() == 1){
                        inSyncList.insert(app);
                    }
                    else{
                        outofSyncList.insert(app);
                    }
                }
            }

            std::cout << "Status:" << std::endl;
            
            for(const auto& app : inSyncList){ // Up to date apps
                std::cout << ANSI_COLOR_GREEN << "In sync: " << app << ANSI_COLOR_RESET << std::endl;
            }

            for(const auto& app : outofSyncList){ // Out of sync apps
                std::cout << ANSI_COLOR_YELLOW << "Out of sync: " << app << ANSI_COLOR_RESET << std::endl;
            }

            for(const auto& app : neverSaveList){ // Never synced apps
                std::cout << ANSI_COLOR_RED << "Never synced: " << app << ANSI_COLOR_RESET << std::endl;
            }

        }


        else if(std::string(argv[2]) == "Jackett" || std::string(argv[2]) == "jackett"){ // Jackett @subparam

            programconfig pcfg("Jackett", exePath); // Init class
            analyzer anly(pcfg.get_config_paths(), "Jackett", exePath); // Init class
            
            std::cout << "Program: Jackett" << std::endl;
            std::cout << "Last Save: " << anly.get_newest_backup_path() << std::endl;
            
            
            if(anly.is_archive_empty() == 1){ // Check if save exists
                std::cout << "Error: No previous save exists for Jackett." << std::endl;
            }
            else{ // Save exists

                if(anly.is_identical_config() == 1){ // Compare config
                    std::cout << ANSI_COLOR_GREEN << "Config is up to date!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_RED << "Config is out of sync!" << ANSI_COLOR_RESET << std::endl;
                }
            }
        }


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr @subparam

            programconfig pcfg("Prowlarr", exePath); // Init class
            analyzer anly(pcfg.get_config_paths(), "Prowlarr", exePath); // Init class
            
            std::cout << "Program: Prowlarr" << std::endl;
            std::cout << "Last Save: " << anly.get_newest_backup_path() << std::endl;
            
            
            if(anly.is_archive_empty() == 1){ // Check if save exists
                std::cout << "Error: No previous save exists for Prowlarr." << std::endl;
            }
            else{ // Save exists

                if(anly.is_identical_config() == 1){ // Compare config
                    std::cout << ANSI_COLOR_GREEN << "Config is up to date!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_RED << "Config is out of sync!" << ANSI_COLOR_RESET << std::endl;
                }
            }
        }


        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent @subparam
 
            programconfig pcfg("qBittorrent", exePath); // Init class
            analyzer anly(pcfg.get_config_paths(), "qBittorrent", exePath); // Init class
            
            std::cout << "Program: qBittorrent" << std::endl;
            std::cout << "Last Save: " << anly.get_newest_backup_path() << std::endl;
            
            
            if(anly.is_archive_empty() == 1){ // Check if save exists
                std::cout << "Error: No previous save exists for qBittorrent." << std::endl;
            }
            else{ // Save exists

                if(anly.is_identical_config() == 1){ // Compare config
                    std::cout << ANSI_COLOR_GREEN << "Config is up to date!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_RED << "Config is out of sync!" << ANSI_COLOR_RESET << std::endl;
                }
            }
        }
    }


    else if(std::string(argv[1]) == "show" || std::string(argv[1]) == "--show"){ // 'show' @param
        
        if(argv[2] == NULL){ // default. No subparam provided
            std::cerr << "Fatal: missing program argument." << std::endl;
            std::cout << "See 'cfgs --help'." << std::endl;
        }

        
        else if(std::string(argv[2]) == "Jackett" || std::string(argv[2]) == "jackett"){ // Jackett @subparam

            programconfig pcfg("Jackett", exePath); // Init class
            analyzer anly(pcfg.get_config_paths(), "Jackett", exePath); // Init class

            
            if(anly.is_archive_empty() == 1) {
                std::cout << "Archive does not contain previous saves of Jackett." << std::endl;
                std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
            }
            else{
                std::vector<std::string> saveList;
                anly.get_all_saves(saveList);
                anly.sortby_filename(saveList);
    
                std::cout << "Showing Jackett:" << std::endl;
                
                int i = 1;
                for(const auto& save : saveList){ // Show all saves
                    std::cout << i << ". " << save << std::endl;
                    i++;
                }
                
                std::cout << "Found " << saveList.size() << " saves;" << std::endl;
            }
        }


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr @subparam
            programconfig pcfg("Prowlarr", exePath); // Init class
            analyzer anly(pcfg.get_config_paths(), "Prowlarr", exePath); // Init class

            
            if(anly.is_archive_empty() == 1) {
                std::cout << "Archive does not contain previous saves of Prowlarr." << std::endl;
                std::cout << "Use 'cfgs --sync prowlarr' to create one." << std::endl;
            }
            else{
                std::vector<std::string> saveList;
                anly.get_all_saves(saveList);
                anly.sortby_filename(saveList);
    
                std::cout << "Showing Prowlarr:" << std::endl;
                
                int i = 1;
                for(const auto& save : saveList){ // Show all saves
                    std::cout << i << ". " << save << std::endl;
                    i++;
                }
                
                std::cout << "Found " << saveList.size() << " saves;" << std::endl;
            }
        }


        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent @subparam
            programconfig pcfg("qBittorrent", exePath); // Init class
            analyzer anly(pcfg.get_config_paths(), "qBittorrent", exePath); // Init class

            
            if(anly.is_archive_empty() == 1) { // Archive empty
                std::cout << "Archive does not contain previous saves of qBittorrent." << std::endl;
                std::cout << "Use 'cfgs --sync qbittorrent' to create a save." << std::endl;
            }
            else{ // Archive not empty
                std::vector<std::string> saveList;
                anly.get_all_saves(saveList);
                anly.sortby_filename(saveList);
    
                std::cout << "Showing qBittorrent:" << std::endl;
                
                int i = 1;
                for(const auto& save : saveList){ // Show all saves
                    std::cout << i << ". " << save << std::endl;
                    i++;
                }
                
                std::cout << "Found " << saveList.size() << " saves;" << std::endl;
            }
        }
    }


    else if(std::string(argv[1]) == "list" || std::string(argv[1]) == "--list"){ // 'list' @param

        std::cout << "Supported Programs: " << std::endl;

        unsigned int i = 1;
        for(const auto& app : programconfig::get_support_list()){
            std::cout << i << ". " << app << std::endl;
            i++;
        }
    }


    else if(std::string(argv[1]) == "settings" || std::string(argv[1]) == "--settings"){ // 'settings' @param 
        
        if(argv[2] == NULL){ //* Default. No subparam provided.
            std::cout << ANSI_COLOR_YELLOW << "Settings: " << ANSI_COLOR_RESET << std::endl;

            std::cout << "Config save limit:            " << ANSI_COLOR_50 << pt.get<int>("SAVELIMIT") << std::endl;
            std::cout << "See 'cfgs settings.save <limit>" << std::endl << std::endl;
            
            std::cout << "Backup recycle bin limit:     " << ANSI_COLOR_50 << pt.get<int>("RECYCLELIMIT") << std::endl;
            std::cout << "See 'cfgs settings.recycle <limit>" << std::endl << std::endl;

            std::cout << "Scheduled Task On/Off:        " << ANSI_COLOR_50 << pt.get<int>("scheduledTask") << std::endl;
            std::cout << "See 'cfgs settings.task <On/Off>" << std::endl << std::endl;

            std::cout << "Scheduled Task Frequency:     " << ANSI_COLOR_50 << pt.get<int>("scheduledTaskFrequency") << std::endl;
            std::cout << "See 'cfgs settings.schedule <On/Off>" << std::endl << std::endl;

            std::cout << "For reset, see 'cfgs settings.reset'" << std::endl;
        }


        else if(std::string(argv[2]) == "--json" || std::string(argv[2]) == "json"){ //* 'json' @subparam
        
            std::string codeComm = "cmd /c code \"" + exePath + "\\config.json\"";

            if(system(codeComm.c_str()) != 0){ // Open config with vscode
                std::string notepadComm = "cmd /c notepad \"" + exePath + "\\config.json\"";
                std::system(notepadComm.c_str()); // Open with notepad
            }
        }
        
    }
    
    return 0;
}