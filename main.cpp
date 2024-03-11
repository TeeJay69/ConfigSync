#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <map>
#include <chrono>
#include <thread>
#include <set>
#include <cctype>
#include <csignal>
#include <windows.h>
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
#define VERSION "v1.0.0"


volatile sig_atomic_t interrupt = 0;

void enableColors(){
    DWORD consoleMode;
    HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(outputHandle, &consoleMode))
    {
        SetConsoleMode(outputHandle, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

void exitSignalHandler(int signum){
    if(interrupt == 0){
        interrupt = 1;
        std::cout << "Warning: Ctrl+C was pressed. Please remain calm and wait for the program to finish.\n";
        std::signal(SIGINT, exitSignalHandler); // Reset SIGINT flag
    }
    else{
        static int count = 2;
        std::cout << ANSI_COLOR_226 << "WARNING: Prematurely killing this process can corrupt the targeted programs!" << ANSI_COLOR_RESET << std::endl;

        if(count < 9){
            std::signal(SIGINT, exitSignalHandler); // Reset SIGINT flag
        }
        else{
            std::exit(signum);
        }
    }
}

int path_check(const std::vector<std::string>& paths){
    for(const auto& item : paths){ // Check if program exist
        std::cerr << "Debug m9\n";
        if(!std::filesystem::exists(item)){
            std::cerr << "Debug m10\n";
            // std::cerr << "Program path not found: (" << item << ").\n" << std::endl;
            return 0;
        }
        std::cerr << "DEBUG m11\n";
    }
    std::cerr << "Debug m 12\n";
    return 1;
}

// TODO: Add optional parameter for the group_id's. 
int create_save(const std::vector<std::string>& programPaths, const std::string& program, const std::string& pArchivePath, const std::string& exelocation){
    if(path_check(programPaths) == 1){ // Verify program paths        
        std::cout << "DEBUG m82\n";
        analyzer configAnalyzer(programPaths, program, exelocation);
        std::cout << "DEBUG m84\n";

        if(configAnalyzer.is_archive_empty() == 1){ // First ever save.
            std::cout << "DEBUG m87\n";
            std::cout << "First synchronization of " << program << "." << std::endl;
            synchronizer sync(programPaths, program, exelocation); // initialize class

            if(sync.copy_config(pArchivePath, synchronizer::ymd_date()) != 1){ // sync config   
                std::cout << "DEBUG m92\n";
                std::cerr << "Error synchronizing " + program << "." << std::endl; // copy_config returned 0
                return 0;
            }
            std::cout << "DEBUG m96\n";
        }
        else{
            std::cout << "DEBUG m99\n";
            if(configAnalyzer.is_identical_config()){ // Compare last save to current config
                std::cout << program + " config did not change, updating save date..." << std::endl;
                std::cout << "DEBUG m102\n";
                
                std::filesystem::path newestPath = configAnalyzer.get_newest_backup_path();
                std::filesystem::rename(newestPath, newestPath.parent_path().string() + "\\" + synchronizer::ymd_date()); // Update name of newest save dir

                return 1;
            }
            else{ // Config changed
                std::cout << "DEBUG m110\n";
                std::cout << program << " config changed.\nSynchronizing " << program << "..." << std::endl;

                synchronizer sync(programPaths, program, exelocation); // initialize class

                std::cout << "DEBUG m115\n";
                if(sync.copy_config(pArchivePath, synchronizer::ymd_date()) != 1){ // sync config   
                    std::cout << "DEBUG m117\n";
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

    std::cout << "DEBUG m129\n";
    return 1;
}

/**
 * @brief Functions for managing scheduled tasks.
 * 
 */
class task{
    public:
        /**
         * @brief Creates a new task in the windows task scheduler.
         * @param taskname The name for the scheduled task.
         * @param target The file path to the target of the task.
         * @param taskfrequency The intervall and frequency of the scheduled task. Ftring format: '<daily/hourly>,<value>'
         * @return 1 for success. 0 for error.
         */
        static const int createtask(const std::string& taskname, const std::string& target, const std::string& taskfrequency){
            
            std::stringstream ss(taskfrequency);
            std::string token;

            const std::string timeframe;
            const std::string intervall;
            std::vector<std::string> list;

            while(getline(ss, token, ',')){
                list.push_back(token);
            }

            const std::string com = "cmd /c \"schtasks /create /tn \"" + taskname + "\" /tr \"\\\"" + target + "\\\"sync\" /sc " + list[0] + " /mo " + list[1] + " >NUL 2>&1\"";

            if(std::system(com.c_str()) != 0){
                return 0;
            }

            return 1;
        }


        /**
         * @brief Removes a task from the windows task scheduler.
         * @param taskname The name as identifier for the task.
         * @return 1 for success. 0 for error.
         */
        static const int removetask(const std::string& taskname){

            std::string com = "cmd /c \"schtasks /delete /tn " + taskname + " >NUL 2>&1\"";

            if(std::system(com.c_str()) != 0){
                return 0;
            }

            return 1;
        }

        /**
         * @brief Status check for scheduled tasks.
         * @param taskname The name as identifier for the task.
         * @return 1 if the task exists, 0 if it does not exist.
         */
        static const int exists(const std::string& taskname){

            const std::string com = "cmd /c \"schtasks /query /tn " + taskname + " >NUL 2>&1\"";

            if(std::system(com.c_str()) != 0){
                return 0;
            }

            return 1;
        }
};


/** 
 * @brief Default setting values
 * @note Strings must be lowercase.
 *
 */
class defaultsetting{
    public:

        static const int savelimit(){
            return 3;
        }

        static const int recyclelimit(){
            return 3;
        }

        static const bool task(){
            return false;
        }

        static const std::string taskfrequency(){
            return "daily,1";
        }


        /**
         * @brief Verify the integrity of the settings file.
        */
        static const int integrity_check(const std::string& path){
            return 1;
        }
};


int main(int argc, char* argv[]){

    std::signal(SIGINT, exitSignalHandler);
    enableColors();
    // Get location of exe
    boost::filesystem::path exePathBfs(boost::filesystem::initial_path<boost::filesystem::path>());
    exePathBfs = (boost::filesystem::system_complete(boost::filesystem::path(argv[0])));

    const std::string exePath = exePathBfs.parent_path().string();
    const std::string settingsPath = exePath + "\\settings.json";
    const std::string batPath = exePath + "\\cfgs.bat";
    const std::string taskName = "ConfigSyncTask";

    // Settings @section
    boost::property_tree::ptree pt;

    try{ // Try loading the file. Its at the same time a validity check
        boost::property_tree::read_json(settingsPath, pt); // read config
    }

    catch(const boost::property_tree::json_parser_error& readError){ // Create default settings file.
        
        if(!std::filesystem::exists(settingsPath)){
            std::cout << "Creating settings file..." << std::endl;
        }
        else{
            std::cerr << "Warning: error while parsing settings file (" << readError.what() << "). Restoring default settings..." << std::endl;
        }
        
        pt.put("settingsID", SETTINGS_ID); // Settings identifier for updates
        pt.put("recyclelimit", defaultsetting::recyclelimit());
        pt.put("savelimit", defaultsetting::savelimit());
        pt.put("task", defaultsetting::task());
        pt.put("taskfrequency", defaultsetting::taskfrequency());


        try{
            std::cout << "Writing settings file..." << std::endl;
            boost::property_tree::write_json(settingsPath, pt);
            std::cout << "Settings file created.\n" << std::endl;
        }
        catch(const boost::property_tree::json_parser_error& writeError){
            std::cerr << "Error: Writing default settings to file failed! (" << writeError.what() << "). Terminating program!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        
        try{ // Writing was successfull, now we read them back.
            boost::property_tree::read_json(settingsPath, pt); // read config
        }
        catch(const boost::property_tree::json_parser_error& doubleReadError){
            std::cerr << "Error: Reading from file after writing default settings failed! [" << __FILE__ << "] [" << __LINE__ << "]. (" << doubleReadError.what() << "). Terminating program!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    

    int settingsID = pt.get<int>("settingsID");

    if(settingsID == 0){ // Failed to retrieve settingsID (file might be corrupt)
        std::cerr << "Error: Failed to extract settingsID from " << settingsPath << std::endl;
        std::cout << "Restoring settings file..." << std::endl;
        
        pt.put("settingsID", SETTINGS_ID); // Settings identifier

        pt.put("recyclelimit", defaultsetting::recyclelimit());
        pt.put("savelimit", defaultsetting::savelimit());
        pt.put("task", defaultsetting::task());
        pt.put("taskfrequency", defaultsetting::taskfrequency());

    }

    else if(settingsID != SETTINGS_ID){ // Differing settingsID
        //! merge config.
        // Write this section when you create a version that made changes to the config file.
        // Create the new config file after merging.
    }
    else{
        // Do nothing, pt contains correct config
    }


    /* Ensure that task setting reflects reality. */
    if(pt.get<bool>("task") == true && !task::exists(taskName)){ // Task setting is true and task doesnt exist
        if(task::createtask(taskName, batPath, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);} // Create task
    }

    else if(pt.get<bool>("task") == false && task::exists(taskName)){ // Task setting is false and task exists
        if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);} // Remove existing task
        std::exit(EXIT_FAILURE);
    }
    
    else if(pt.get<bool>("task") == false && !task::exists(taskName)){
        // Do nothing.
    }
    
    else if(pt.get<bool>("task") == true && task::exists(taskName)){
        // Do nothing.
    }
    
    else{
        std::cerr << "File: [" << __FILE__ << "] Line: [" << __LINE__ << "] This should never happen!\n";
    }



    /* Parse command line options: */
     
    if(argv[1] == NULL){ // No params, display Copyright Notice
        std::cout << "ConfigSync (JW-Coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber. All rights reserved." << std::endl;
        std::cout << "Synchronize program configurations." << std::endl;
        std::cout << "See 'cfgs --help' for usage." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    else if(std::string(argv[1]) == "version" || std::string(argv[1]) == "--version"){ // Version param
        std::cout << "ConfigSync (JW-Coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber. All rights reserved." << std::endl;
        std::cout << "Synchronize program configurations." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    else if(std::string(argv[1]) == "help" || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"){ // Help message param
        std::cout << "ConfigSync (JW-Coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::cout << "usage: cfgs [OPTIONS]... [PROGRAM]" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:\n";
        std::cout << "sync [PROGRAM]                Create a snapshot of a program's configuration.\n";
        std::cout << "sync --all                    Create a snapshot for all supported programs.\n";
        std::cout << "restore [PROGRAM] [DATE]      Restore configuration of the specified program, date.\n";
        std::cout << "restore --all                 Restore configuration of all supported programs.\n";
        std::cout << "status [PROGRAM]              Display synchronization state. (Default: all)\n";
        std::cout << "show [PROGRAM]                List all existing snapshots of a program\n";
        std::cout << "list --supported              Show all supported programs.\n";
        std::cout << "settings                      Show settings.\n";
        std::cout << "settings --json               Show settings (JSON).\n";
        std::cout << "settings.reset [SETTING]      Reset one or multiple settings.\n";
        std::cout << "settings.reset --all          Reset all settings to default.\n";
        std::cout << "settings --help               Show detailed settings info.\n";
        std::cout << "--help                        Display help message.\n";
        std::cout << "--version                     Display version and copyright disclaimer.\n";
        std::exit(EXIT_SUCCESS);
    }
    


    else if(std::string(argv[1]) == "sync"){ // Sync param

        if(argv[2] == NULL){ // No subparam provided
            std::cout << "Fatal: Missing program or restore argument." << std::endl;
        }
        
        else if(std::string(argv[2]) == "--all"){ // Create a save of all supported, installed programs. No subparam provided
            std::cout << ANSI_COLOR_161 << "Synchronizing all programs:" << ANSI_COLOR_RESET << std::endl;
            
            std::cout << ANSI_COLOR_222 << "Fetching paths..." << ANSI_COLOR_RESET << std::endl;

            // Get program paths
            programconfig jackett("Jackett", exePath);
            std::vector<std::string> jackettPaths = jackett.get_config_paths();
            
            programconfig prowlarr("Prowlarr", exePath);
            std::vector<std::string> prowlarrPaths = prowlarr.get_config_paths();

            programconfig qbittorrent("qBittorrent", exePath);
            std::vector<std::string> qbittorrentPaths = qbittorrent.get_config_paths();
            
            std::cout << ANSI_COLOR_222 << "Checking archive..." << ANSI_COLOR_RESET << std::endl;

            // Create the archive @dir if it does not exist yet
            std::cout << "DEBUG: 404: \n";
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

            std::cout << ANSI_COLOR_222 << "Starting synchronization..." << ANSI_COLOR_RESET << std::endl;

            organizer janitor; // Initialize class

            // Sync Jackett
            std::cout << "DEBUG 424: " << std::endl;
            int jackettSync = 0;
            if(create_save(jackettPaths, "Jackett", jackett.get_archive_path().string(), exePath) == 1){
                std::cout << "DEBUG 426: " << std::endl;
                jackettSync = 1;
                syncedList.push_back("Jackett");

                // Limit number of saves
                std::cout << "DEBUG 432: " << std::endl;
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), jackett.get_archive_path().string()); // Cleanup
                std::cout << "DEBUG 434: " << std::endl;
            }

            // Sync Prowlarr
            int prowlarrSync = 0;
            if(create_save(prowlarrPaths, "Prowlarr", prowlarr.get_archive_path().string(), exePath) == 1){
                prowlarrSync = 1;
                syncedList.push_back("Prowlarr");

                // Limit number of saves
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), prowlarr.get_archive_path().string()); // Cleanup
            }

            // Sync qBittorrent
            int qbittorrentSync = 0;
            if(create_save(qbittorrentPaths, "qBittorrent", qbittorrent.get_archive_path().string(), exePath) == 1){
                qbittorrentSync = 1;
                syncedList.push_back("qBittorrent");

                // Limit number of saves
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), qbittorrent.get_archive_path().string()); // Cleanup
            };
            
            std::cout << ANSI_COLOR_222 << "Preparing summary..." << ANSI_COLOR_RESET << std::endl;

            // Info
            std::cout << "Synced programs:" << std::endl;

            int i = 1;
            for(const auto& item : syncedList){
                std::cout << ANSI_COLOR_GREEN << "      " << i << "." << item << ANSI_COLOR_RESET << std::endl;
                i++;
            }
            std::cout << ANSI_COLOR_GREEN << "Synchronization finished!" << ANSI_COLOR_RESET << std::endl;
        }

        else if(std::string(argv[2]) == "jackett" || std::string(argv[2]) == "Jackett"){ // Jackett sync subparam
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
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), jackett.get_archive_path().string()); // Cleanup

                std::cout << ANSI_COLOR_GREEN << "Synchronization of Jackett finished." << ANSI_COLOR_RESET << std::endl;
            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Jackett failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(std::string(argv[2]) == "prowlarr" || std::string(argv[2]) == "Prowlarr"){ // Prowlarr sync subparam
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
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), prowlarr.get_archive_path().string()); // Cleanup

                std::cout << ANSI_COLOR_GREEN << "Synchronization of Prowlarr finished." << ANSI_COLOR_RESET << std::endl;

            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Prowlarr failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){  // qBittorrent sync subparam
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
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), qbittorrent.get_archive_path().string()); // Cleanup

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


    else if(std::string(argv[1]) == "restore" || std::string(argv[1]) == "--restore"){ // Restore param

        if(argv[2] == NULL){ // No program provided. Display advice. No subparam provided.
            std::cout << "Fatal: Missing program or restore argument." << std::endl;
            std::cout << "For usage see 'cfgs --help'" << std::endl;
        }

        
        else if(std::string(argv[2]) == "all" || std::string(argv[2]) == "--all"){ // '--all' subparam
            std::cout << ANSI_COLOR_YELLOW << "Restoring all supported programs with latest save..." << ANSI_COLOR_RESET << std::endl;
            
            std::vector<std::string> failList;

            // @section Jackett:
            programconfig jackett("Jackett", exePath); // Initialize class
            std::vector<std::string> jackettPaths = jackett.get_config_paths(); // Get program paths
            
            analyzer anlyJackett(jackettPaths, "Jackett", exePath); // Initialize class
            synchronizer syncJackett(jackettPaths, "Jackett", exePath); // Initialize class
            std::cout << "Debug m574\n";
            int jackettState = 0;
            if(anlyJackett.is_archive_empty() == 1){ // Archive is empty
                std::cout << "Debug m577\n";
                std::cout << "Skipping Jackett, not synced..." << std::endl;
                failList.push_back("Jackett");
            }
            else{ // Archive not empty
                if(syncJackett.restore_config(anlyJackett.get_newest_backup_path(), pt.get<int>("recyclelimit")) == 1){ // Restore from newest save
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
            std::vector<std::string> prowlarrPaths = prowlarr.get_config_paths(); // Get program paths
            
            analyzer anlyProw(prowlarrPaths, "Prowlarr", exePath); // Initialize class
            synchronizer syncProw(prowlarrPaths, "Prowlarr", exePath); // Initialize class

            int prowlarrState = 0;
            if(anlyProw.is_archive_empty() == 1){ // Archive empty
                std::cout << "Skipping Prowlarr, not synced..." << std::endl;
                failList.push_back("Prowlarr");
            }
            else{ // Archive not empty
                if(syncProw.restore_config(anlyProw.get_newest_backup_path(), pt.get<int>("recyclelimit")) == 1){ // Restore from newest save
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
            std::vector<std::string> qbittorrentPaths = qbittorrent.get_config_paths(); // Get program paths
            
            analyzer anlyQbit(qbittorrentPaths, "qBittorrent", exePath); // Initialize class
            synchronizer syncQbit(qbittorrentPaths, "qBittorrent", exePath); // Initialize class
            
            int qbitState = 0;
            if(anlyQbit.is_archive_empty() == 1){ // Archive empty
                std::cout << "Skipping qBittorrent, not synced..." << std::endl;
                failList.push_back("qBittorrent");
            }
            else{ // Archive not empty
                if(syncQbit.restore_config(anlyQbit.get_newest_backup_path(), pt.get<int>("recyclelimit")) == 1){ // Restore from newest save
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


        else if(std::string(argv[2]) == "Jackett" || std::string(argv[2]) == "jackett"){ // Jackett subparam
            
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

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit")) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }

                }
            }


            else if(std::find(jackettSaves.begin(), jackettSaves.end(), std::string(argv[3])) != jackettSaves.end()){ // Specific save date 2subparam. Check if date is valid.

                synchronizer sync(pPaths, "Jackett", exePath); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("recyclelimit")) == 1){ // Restore from user defined save date<
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


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr subparam
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

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit")) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }


            else if(std::find(prowlarrSaves.begin(), prowlarrSaves.end(), std::string(argv[3])) != prowlarrSaves.end()){ // Specific save date 2subparam. Check if date is valid.

                synchronizer sync(pPaths, "Prowlarr", exePath); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("recyclelimit")) == 1){ // Restore from user defined save date
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
        

        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent subparam
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

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit")) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }


            else if(std::find(qbittorrentSaves.begin(), qbittorrentSaves.end(), std::string(argv[3])) != qbittorrentSaves.end()){ // Specific save date 2subparam. Check if date is valid.

                synchronizer sync(pPaths, "qBittorrent", exePath); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("recyclelimit")) == 1){ // Restore from user defined save date
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

        
        else{
            std::cerr << "Fatal: invalid argument. See 'cfgs --help'.\n";
        }
    }


    else if(std::string(argv[1]) == "status"){ // 'status' param

        if(argv[2] == NULL){ // default: all. subparam

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
                std::cout << ANSI_COLOR_GREEN << "Up to date: " << app << ANSI_COLOR_RESET << std::endl;
            }

            for(const auto& app : outofSyncList){ // Out of sync apps
                std::cout << ANSI_COLOR_YELLOW << "Out of sync: " << app << ANSI_COLOR_RESET << std::endl;
            }

            for(const auto& app : neverSaveList){ // Never synced apps
                std::cout << ANSI_COLOR_RED << "Never synced: " << app << ANSI_COLOR_RESET << std::endl;
            }

        }


        else if(std::string(argv[2]) == "Jackett" || std::string(argv[2]) == "jackett"){ // Jackett subparam

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


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr subparam

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


        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent subparam
 
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

        
        else{
            std::cerr << "Fatal: invalid argument. See 'cfgs --help'.\n";
        }
    }


    else if(std::string(argv[1]) == "show" || std::string(argv[1]) == "--show"){ // 'show' param
        
        if(argv[2] == NULL){ // default. No subparam provided
            std::cerr << "Fatal: missing program argument." << std::endl;
            std::cout << "See 'cfgs --help'." << std::endl;
        }

        
        else if(std::string(argv[2]) == "Jackett" || std::string(argv[2]) == "jackett"){ // Jackett subparam

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


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr subparam
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


        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent subparam
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


        else{
            std::cerr << "Fatal: invalid argument. See 'cfgs --help'.\n";
        }
    }


    else if(std::string(argv[1]) == "list"){ // 'list' param

        std::cout << "Supported Programs: " << std::endl;

        unsigned int i = 1;
        for(const auto& app : programconfig::get_support_list()){
            std::cout << i << ". " << app << std::endl;
            i++;
        }
    }


    else if(std::string(argv[1]) == "settings"){ // 'settings' param 
        
        if(argv[2] == NULL){ //* Default. No subparam provided.
            std::cout << ANSI_COLOR_YELLOW << "Settings: " << ANSI_COLOR_RESET << std::endl;

            std::cout << "Config save limit:            " << ANSI_COLOR_50 << pt.get<int>("savelimit") << std::endl;
            std::cout << "See 'cfgs settings.savelimit <limit>" << std::endl << std::endl;
            
            std::cout << "Backup recycle bin limit:     " << ANSI_COLOR_50 << pt.get<int>("recyclelimit") << std::endl;
            std::cout << "See 'cfgs settings.recyclelimit <limit>" << std::endl << std::endl;

            std::cout << "Scheduled Task On/Off:        " << ANSI_COLOR_50 << pt.get<bool>("task") << std::endl;
            std::cout << "See 'cfgs settings.task <True/False>" << std::endl << std::endl;

            std::cout << "Scheduled Task Frequency:     " << ANSI_COLOR_50 << pt.get<std::string>("taskfrequency") << std::endl;
            std::cout << "See 'cfgs settings.taskfrequency <hourly/daily> <hours/days>" << std::endl << std::endl;

            std::cout << "For reset, see 'cfgs settings.reset'" << std::endl;
        }

        else if(std::string(argv[2]) == "--help"){
            std::cout << "Settings options overview:\n";
            std::cout << "Usage: settings.[SETTING] [VALUE]\n";
            std::cout << "task [True/False]                           Snapshot task for all programs.\n";
            std::cout << "taskfrequency [hourly/daily] [hours/days]   Intervall of scheduled task.\n";
            std::cout << "savelimit [1-400]                           Maximum retained snapshots.\n";
            std::cout << "recyclelimit [1-400]                        Maximum retained backup snapshots from restoring.\n";
        }

        else if(std::string(argv[2]) == "--json"){ //* 'json' subparam

            std::cout << "Warning: any changes to the json file could make the file unreadable, please know what you are doing!" << std::endl; // Warning
            int counter = 2;
            while(counter != 0){
                if(counter == 1){
                    std::cout << "\rProceeding in: " << counter << " second" << std::flush;
                }
                std::cout << "\rProceeding in: " << counter << " seconds" << std::flush;

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                counter--;
            }
            
            const std::string codeComm = "cmd /c code \"" + exePath + "\\config.json\"";

            if(system(codeComm.c_str()) != 0){ // Open config with vscode
                const std::string notepadComm = "cmd /c notepad \"" + exePath + "\\config.json\"";
                std::system(notepadComm.c_str()); // Open with notepad
            }

        }
        
        else{
            std::cerr << "Fatal: invalid argument. See 'cfgs --help'.\n";
        }
    }


    else if(std::string(argv[1]) == "settings.reset" || std::string(argv[1]) == "--settings.reset"){ // 'settings.reset` param
        
        if(argv[2] == NULL){ // default. No subparam provided.
            std::cout << "Fatal: missing value for argument. See 'cfgs --settings'." << std::endl;
        }


        else if(std::string(argv[2]) == "savelimit" || std::string(argv[2]) == "--savelimit"){ // 'savelimit' subparam 

            std::cout << "SaveLimit reset to default." << std::endl;
            std::cout << "(" << pt.get<int>("savelimit") << ") ==> (" << defaultsetting::savelimit() << ")" << std::endl;

            pt.put("savelimit", defaultsetting::savelimit()); // Reset

            boost::property_tree::write_json(settingsPath, pt); // Update config file
        }

        
        else if(std::string(argv[2]) == "recyclelimit" || std::string(argv[2]) == "--recyclelimit"){ // 'recyclelimit' subparam

            std::cout << "RecycleLimit reset to default." << std::endl;
            std::cout << "(" << pt.get<int>("recyclelimit") << ") ==> (" << defaultsetting::recyclelimit() << ")" << std::endl;

            pt.put("recyclelimit", defaultsetting::recyclelimit()); // Reset

            boost::property_tree::write_json(settingsPath, pt); // Update config file
        }

        
        else if(std::string(argv[2]) == "task" || std::string(argv[2]) == "--task"){ // 'task' subparam

            std::cout << "Task reset to default." << std::endl;
            std::cout << "(" << pt.get<bool>("task") << ") ==> (" << defaultsetting::task() << ")" << std::endl;

            pt.put("task", defaultsetting::task()); // Reset
            boost::property_tree::write_json(settingsPath, pt); // Update config file
            

            if(defaultsetting::task() == false && task::exists(taskName) == 1){ // Defaultsetting for 'task' is false and task exists
                if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);} // Remove the task
            }

            else if(defaultsetting::task() == true && task::exists(taskName) == 0){ // Defaultsetting for 'task' is true and task doesnt yet exist.
                task::createtask(taskName, batPath, defaultsetting::taskfrequency()); // Create the task
            }
            else{std::cerr << "File: [" << __FILE__ << "] Line: [" << __LINE__ << "] This should never happen!\n";}
        }


        else if(std::string(argv[2]) == "taskfrequency" || std::string(argv[2]) == "--taskfrequency"){ // 'taskfrequency' subparam

            std::cout << "TaskFrequency reset to default." << std::endl;
            std::cout << "(" << pt.get<std::string>("taskfrequency") << ") ==> (" << defaultsetting::taskfrequency() << ")" << std::endl;

            pt.put("taskfrequency", defaultsetting::taskfrequency()); // Reset
            boost::property_tree::write_json(settingsPath, pt); // Update config file


            if(task::exists(taskName) == 1){ // Task exists.
                if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);}
                task::createtask(taskName, batPath, defaultsetting::taskfrequency()); // Create task to apply the reset taskfrequeny
            }
            else if(pt.get<bool>("task") == true){ // Task does not exist but tasksetting is on. 
                task::createtask(taskName, batPath, defaultsetting::taskfrequency()); // Create task
            }
            else{std::cerr << "File: [" << __FILE__ << "] Line: [" << __LINE__ << "] This should never happen!\n";}
        }
        

        else if(std::string(argv[2]) == "--all"){ // '--all' subparam

            pt.put("savelimit", defaultsetting::savelimit());
            pt.put("recyclelimit", defaultsetting::recyclelimit());
            pt.put("task", defaultsetting::task());
            pt.put("taskfrequency", defaultsetting::taskfrequency());

            boost::property_tree::write_json(settingsPath, pt); // Update config file
            

            if(defaultsetting::task() == true && !task::exists(taskName)){ // Task default is 'true' and task doesnt exist.

                task::createtask(taskName, batPath, defaultsetting::taskfrequency()); // Create the task
            }

            else if(defaultsetting::task() == false && task::exists(taskName)){ // Task default is 'false' and task exists.

                if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);} // Remove task
            }
            else{
                std::cerr << "File: [" << __FILE__ << "] Line: [" << __LINE__ << "] This should never happen!\n";
            }

            std::cout << "All settings reset to default!" << std::endl;
        }

        else{
            std::cerr << "Fatal: invalid argument. See 'cfgs --settings'.\n";
        }
    }


    else if(std::string(argv[1]) == "settings.savelimit" || std::string(argv[1]) == "--settings.savelimit"){ // 'settings.savelimit' param

        if(argv[2] == NULL){ // Default. No subparam.
            std::cout << "Fatal: missing value for argument. See 'cfgs --settings'" << std::endl;
        }
        
        else if(std::stoi(argv[2]) <= 400){ // Value provided.
        
            pt.put("savelimit", std::stoi(argv[2])); // Assign new limit
            boost::property_tree::write_json(settingsPath, pt); // Update config file
        }
        
        else{ // Invalid value
            std::cout << "Fatal: value out of range." << std::endl;
        }
    }
    

    else if(std::string(argv[1]) == "settings.recyclelimit"){ // 'settings.recyclelimit' param

        if(argv[2] == NULL){ // Default. No subparam.
            std::cout << "Fatal: missing value for argument. See 'cfgs --settings'" << std::endl;
        }
        
        else if(std::stoi(argv[2]) <= 400){ // Value provided.
            pt.put("recyclelimit", std::stoi(argv[2])); // Assign new limit
            boost::property_tree::write_json(settingsPath, pt); // Update config file
        }
        
        else{ // Invalid value
            std::cout << "Fatal: value out of range." << std::endl;
        }
    }


    else if(std::string(argv[1]) == "settings.task"){ // 'settings.task' param
        
        if(argv[2] == NULL){ // Default. No subparam.
            std::cout << "Fatal: missing value for argument. See 'cfgs --settings'" << std::endl;
        }
        
        else if(std::string(argv[2]) == "true" || std::string(argv[2]) == "True"){ // 'true' subparam

            if(pt.get<bool>("task") == false){ // Current property is set to 'false'
                pt.put("task", true); // Set property to true
                boost::property_tree::write_json(settingsPath, pt); // Update config file

                if(task::exists(taskName)){ // Task exists --> relaunch task
                    if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);}
                    if(task::createtask(taskName, batPath, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);}
                }
                else{ // Task doesnt exist --> launch task
                    if(task::createtask(taskName, batPath, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);}
                }
            }

            else{ // Current property is already set to 'true'.
                std::cout << "settings.task already set to true. Checking task..." << std::endl;

                if(task::exists("task")){ // Task exists.
                    std::cout << "Task exists, no changes were made." << std::endl;
                }
                else{ // Task doesnt exist. --> launch task
                    std::cout << "Task doesn't exist. Creating task..." << std::endl;
                    if(task::createtask(taskName, batPath, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);}
                    std::cout << "Task created!" << std::endl;
                }
            }
        }

        else if(std::string(argv[2]) == "false" || std::string(argv[2]) == "False"){ // 'false' subparam
            bool prop = pt.get<bool>("task");

            if(prop == true){ // Current property is set to 'true'
                pt.put("task", false); // Set property to false
                boost::property_tree::write_json(settingsPath, pt); // Update config file
                
                if(task::exists(taskName)){ // Task exists --> remove task.
                    if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);}
                }
                else{ // Task doesnt exist --> create task
                    if(task::createtask(taskName, batPath, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);}
                }
            }

            else{ // Current property is set to 'false'
                std::cout << "settings.task already set to false. Checking task..." << std::endl;

                if(task::exists(taskName)){ // Task exists --> remove task
                    std::cout << "Task exists. Removing task..." << std::endl;
                    if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);}
                    std::cout << "Task removed!" << std::endl;
                }
                else{ // Task doesnt exist
                    std::cout << "Task doesn't exist, no changes were made." << std::endl;
                }
            }
        }
        
        else{ // Invalid argument subparam
            std::cerr << "Fatal: invalid argument. See 'cfgs --settings'." << std::endl;
        }
    }


    else if(std::string(argv[1]) == "settings.taskfrequency"){
        
        if(argv[2] == NULL){ // Default. No subparam.
            std::cout << "Fatal: missing value for argument. See 'cfgs --settings'" << std::endl;
        }
        

        else if(std::string(argv[2]) == "daily" || std::string(argv[2]) == "Daily" || std::string(argv[2]) == "hourly" || std::string(argv[2]) == "Hourly"){ // 'daily' or 'hourly' subparam

            if(argv[3] == NULL){
                std::cout << "Fatal: missing value for argument. See 'cfgs --settings'" << std::endl;
            }

            else if(std::stoi(argv[3]) <= 1000){

                std::string lcase = (std::string)argv[2]; 
                std::transform(lcase.begin(), lcase.end(), lcase.begin(), // Convert to lowercase
                    [](unsigned char c){ return std::tolower(c); });
                    
                const std::string tf =  lcase + "," + (std::string)argv[3];

                pt.put("taskfrequency", tf); // Assign new value
                boost::property_tree::write_json(settingsPath, pt); // Update config file

                if(task::exists(taskName)){
                    if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);}
                    if(task::createtask(taskName, batPath, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);}
                }
                else{
                    // Task doesn exist. do nothing.
                }
            }
            
            else if(std::stoi(argv[3]) > 1000){ // Invalid value
                std::cout << "Fatal: value out of range." << std::endl;
            }
            else{
                std::cerr << "Fatal: invalid argument. See 'cfgs --settings'." << std::endl;
            }
        }
    }


    
    return 0;
}