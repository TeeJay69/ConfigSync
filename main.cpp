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
#include "logs.hpp"
#include "CFGSIndex.hpp"
#include "CFGSExcept.hpp"

#ifdef DEBUG
#define DEBUG_MODE 1
#else 
#define DEBUG_MODE 0
#endif

#define SETTINGS_ID 1
#define VERSION "v1.0.0"

using std::string;

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
        if(!std::filesystem::exists(item)){
            // std::cerr << "Program path not found: (" << item << ").\n" << std::endl;
            return 0;
        }
    }
    return 1;
}

// TODO: Add optional parameter for the group_id's. 
int create_save(const std::vector<std::string>& programPaths, const std::string& program, const std::string& pArchivePath, const std::string& exelocation, std::ofstream& logfile){
    if(path_check(programPaths) == 1){ // Verify program paths        

        analyzer configAnalyzer(programPaths, program, exelocation, logfile);

        if(configAnalyzer.is_archive_empty() == 1){ // First ever save.
            const std::string out = "First synchronization of " + program + ".\n";
            std::cerr << out;
            logfile << logs::ms(out);
            synchronizer sync(program, exelocation, logfile); // initialize class

            if(sync.copy_config(pArchivePath, synchronizer::ymd_date(), programPaths) != 1){ // sync config   
                const std::string errSync = "Error synchronizing " + program + ".\n"; // copy_config returned 0
                std::cerr << errSync;
                logfile << logs::ms(errSync);
                return 0;
            }
        }
        else{
            const std::string dateDir = configAnalyzer.get_newest_backup_path(); // Latest dir

            if(configAnalyzer.has_hashbase(dateDir) == 1){
                if(configAnalyzer.is_identical() == 1){ // Compare last save to current config
                    const std::filesystem::path &newestPath = dateDir;
                    if(newestPath.filename() == synchronizer::ymd_date()){
                        const std::string noChange = program + " config is identical.\n";
                        std::cout << ANSI_COLOR_66 << noChange << ANSI_COLOR_RESET;
                        logfile << logs::ms(noChange);
                    }
                    else{
                        const std::string noChange = program + " config is identical, updating save date...\n";
                        std::cout << ANSI_COLOR_138 << noChange << ANSI_COLOR_RESET;
                        logfile << logs::ms(noChange);
                        std::filesystem::rename(newestPath, newestPath.parent_path().string() + "\\" + synchronizer::ymd_date()); // Update name of newest save dir
                    }

                    return 1;
                }
                else{ // Config changed
                    const std::string changed = program + " config changed. Synchronizing " + program + "...\n";
                    std::cout << ANSI_COLOR_138 << changed << ANSI_COLOR_RESET;
                    logfile << logs::ms(changed);

                    synchronizer sync(program, exelocation, logfile); // initialize class

                    if(sync.copy_config(pArchivePath, synchronizer::ymd_date(), programPaths) != 1){ // sync config   
                        const std::string errSync = "Error synchronizing " + program + ".\n"; // copy_config returned 0
                        std::cerr << errSync;
                        logfile << logs::ms(errSync); 
                        return 0;
                    }
                }
            }

            synchronizer::recurse_remove(dateDir); // Remove the corrupt save, it has no hashbase
            const std::string changed = program + "'s existing save was deleted, due to a missing hashbase. Synchronizing " + program + "...\n";
            std::cout << changed;
            logfile << logs::ms(changed);

            synchronizer sync(program, exelocation, logfile); // initialize class

            if(sync.copy_config(pArchivePath, synchronizer::ymd_date(), programPaths) != 1){ // sync config   
                const std::string errSync = "Error synchronizing " + program + ".\n"; // copy_config returned 0
                std::cerr << errSync;
                logfile << logs::ms(errSync); 
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


int rollback(const string& programName, const std::vector<string>& configPaths, const string& exeLocation, std::ofstream& logfile, std::vector<string>& failList, unsigned restoreLimit){
    analyzer anly(configPaths, programName, exeLocation, logfile); // Initialize class
    synchronizer syncJackett(programName, exeLocation, logfile); // Initialize class

    if(anly.is_archive_empty() == 1){ // Archive is empty

        std::cout << "Skipping " << programName << ", not synced..." << std::endl;
        failList.push_back(programName);
    }
    else{ // Archive not empty
        if(syncJackett.restore_config(anly.get_newest_backup_path(), restoreLimit, configPaths) == 1){ // Restore from newest save
            std::cout << ANSI_COLOR_GREEN << programName << " restore successfull!" << ANSI_COLOR_RESET << std::endl;
            return 1;
        }
        else{
            std::cerr << ANSI_COLOR_RED << programName << " restore failed." << ANSI_COLOR_RESET << std::endl;
            failList.push_back(programName);
            return 0;
        }
    }
}


int revertRestore(const std::string& program, const std::string& exePath, std::ofstream& logfile, std::optional<std::reference_wrapper<std::string>> op_userIn = std::nullopt){
    if(analyzer::has_backup(program, exePath)){
        if(op_userIn.has_value()){
            // Dereference the optional
            auto& op_userInDeref = op_userIn->get();

            std::cout << ANSI_COLOR_166 << "Reverting specified " << program << " restore:" << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Fetching index..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Verifying provided date or UUID..." << ANSI_COLOR_RESET << std::endl;

            // Get index
            Index IX = analyzer::get_Index(program, exePath);

            synchronizer sync(exePath, program, logfile);

            std::string match;
            unsigned hit = 0;
            for(const auto& pair : IX.time_uuid){
                // Convert unsigned long long to string and match for userInput
                if(synchronizer::timestamp_to_string(pair.first).find(op_userInDeref) != std::string::npos){
                    hit++;
                    match = pair.second; // we want uuid
                }
                else if(pair.second.find(op_userInDeref) != std::string::npos){
                    hit++;
                    match = pair.second; // we want uuid
                }
            }

            logfile << logs::ms("revertRestore userInput hits: ") << hit << "Program:" << program << "\n";

            // More than 1 hit, can't allow that...
            if(hit > 1){
                std::cerr << ANSI_COLOR_RED << "Fatal: provided date or uuid was not unique. See 'cfgs show " << program << "'" << ANSI_COLOR_RESET << std::endl;
                return 0;
            }
            else if(hit == 0){
                // No match, cant have that either...
                std::cerr << ANSI_COLOR_RED << "Fatal: provided date or uuid is invalid. See 'cfgs show " << program << "'" << ANSI_COLOR_RESET << std::endl;
                return 0;
            }
            
            std::cout << ANSI_COLOR_222 << "Found a unique match..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Starting revert..." << ANSI_COLOR_RESET << std::endl;
            // Try to undo last restore wit the match uuid
            if(!sync.revert_restore(match) != 1){
                std::cerr << ANSI_COLOR_RED << "Error: Failed to revert last restore.\n";
                logfile << logs::ms("Error: Failed to revert last restore.\n");
                throw cfgsexcept("Please verify that none of your selected programs components are missing or corrupted");
                return 0;
            }

            std::cout << ANSI_COLOR_222 << "Revert finished, cleaning up..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_GREEN << "Revert was successfull!" << ANSI_COLOR_RESET << std::endl;
            return 1;
        }
        else{
            std::cout << ANSI_COLOR_166 << "Reverting last " << program << " restore:" << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Fetching index..." << ANSI_COLOR_RESET << std::endl;
            // Get index
            Index IX = analyzer::get_Index(program, exePath);

            synchronizer sync(exePath, program, logfile);
            
            std::cout << ANSI_COLOR_222 << "Starting revert..." << ANSI_COLOR_RESET << std::endl;
            // Try to undo last restore
            if(!sync.revert_restore(IX.time_uuid.back().second) != 1){
                std::cerr << ANSI_COLOR_RED << "Error: Failed to revert last restore.\n";
                logfile << logs::ms("Error: Failed to revert last restore.\n");
                throw cfgsexcept("Please verify that none of your selected programs components are missing or corrupted");
                return 0;
            }

            std::cout << ANSI_COLOR_222 << "Revert finished, cleaning up..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_GREEN << "Revert was successfull!" << ANSI_COLOR_RESET << std::endl;

            return 1;
        }
    }
    else{
        std::cerr << ANSI_COLOR_RED << "Fatal: no previous restore found. See 'cfgs --help'." << ANSI_COLOR_RESET << std::endl;
    }

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
        static const int savelimit = 3;
        static const int recyclelimit = 3;
        static const bool task = false;
        const std::string taskfrequency = "daily,1";
};


int main(int argc, char* argv[]){


    // Catches exit signal Ctrl+C
    std::signal(SIGINT, exitSignalHandler);
    // Enables colors in windows command processor
    enableColors();
    // Get location of exe
    boost::filesystem::path exePathBfs(boost::filesystem::initial_path<boost::filesystem::path>());
    exePathBfs = (boost::filesystem::system_complete(boost::filesystem::path(argv[0])));

    const std::string exePath = exePathBfs.parent_path().string();
    const std::string settingsPath = exePath + "\\settings.json";
    const std::string batPath = exePath + "\\cfgs.bat";
    const std::string taskName = "ConfigSyncTask";
    std::ofstream logfile = logs::session_log(exePath, 50); // Create logfileile
    
    // Classes
    defaultsetting DS;
    ProgramConfig PC(exePath);

    const auto jackettInfo = PC.get_ProgramInfo("Jackett");
    const auto prowlarrInfo = PC.get_ProgramInfo("Prowlarr");
    const auto qbittorrentInfo = PC.get_ProgramInfo("qBittorrent");

    
    // Settings @section
    boost::property_tree::ptree pt;

    try{ // Try parsing the settings file.
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
        pt.put("recyclelimit", DS.recyclelimit);
        pt.put("savelimit", DS.savelimit);
        pt.put("task", DS.task);
        pt.put("taskfrequency", DS.taskfrequency);


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

        pt.put("recyclelimit", DS.recyclelimit);
        pt.put("savelimit", DS.savelimit);
        pt.put("task", DS.task);
        pt.put("taskfrequency", DS.taskfrequency);

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
        std::cout << "revert [PROGRAM] [DATE]       Revert a restore of the specified program, date.\n";
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


            std::cout << ANSI_COLOR_222 << "Checking archive..." << ANSI_COLOR_RESET << std::endl;
            // Create the archive @dir if it does not exist yet

            if(!std::filesystem::exists(jackettInfo.archivePath)){
                std::filesystem::create_directories(jackettInfo.archivePath);
            }

            if(!std::filesystem::exists(prowlarrInfo.archivePath)){
                std::filesystem::create_directories(prowlarrInfo.archivePath);
            }

            if(!std::filesystem::exists(qbittorrentInfo.archivePath)){
                std::filesystem::create_directories(qbittorrentInfo.archivePath); 
            }        
            
            std::vector<std::string> syncedList; // For sync message

            std::cout << ANSI_COLOR_222 << "Starting synchronization..." << ANSI_COLOR_RESET << std::endl;

            organizer janitor; // Initialize class

            // Sync Jackett
            int jackettSync = 0;
            if(create_save(jackettInfo.configPaths, "Jackett", jackettInfo.archivePath, exePath, logfile) == 1){

                jackettSync = 1;
                syncedList.push_back("Jackett");

                // Limit number of saves

                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), jackettInfo.archivePath); // Cleanup

            }

            // Sync Prowlarr
            int prowlarrSync = 0;
            if(create_save(prowlarrInfo.configPaths, "Prowlarr", prowlarrInfo.archivePath, exePath, logfile) == 1){
                prowlarrSync = 1;
                syncedList.push_back("Prowlarr");

                // Limit number of saves
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), prowlarrInfo.archivePath); // Cleanup
            }

            // Sync qBittorrent
            int qbittorrentSync = 0;
            if(create_save(qbittorrentInfo.configPaths, "qBittorrent", qbittorrentInfo.archivePath, exePath, logfile) == 1){
                qbittorrentSync = 1;
                syncedList.push_back("qBittorrent");

                // Limit number of saves
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), qbittorrentInfo.archivePath); // Cleanup
            };
            
            std::cout << ANSI_COLOR_222 << "Preparing summary..." << ANSI_COLOR_RESET << std::endl;
            // Info
            std::cout << ANSI_COLOR_161 << "Synced programs:" << ANSI_COLOR_RESET << std::endl;
            int i = 1;
            for(const auto& item : syncedList){
                std::cout << ANSI_COLOR_GREEN << i << "." << item << ANSI_COLOR_RESET << std::endl;
                i++;
            }
            std::cout << ANSI_COLOR_GREEN << "Synchronization finished!" << ANSI_COLOR_RESET << std::endl;
        }

        else if(std::string(argv[2]) == "jackett" || std::string(argv[2]) == "Jackett"){ // Jackett sync subparam
            // Create archive if it doesnt exist
            if(!std::filesystem::exists(jackettInfo.archivePath)){
                std::filesystem::create_directories(jackettInfo.archivePath);
            }
            
            
            if(create_save(jackettInfo.configPaths, "Jackett", jackettInfo.archivePath, exePath, logfile) == 1){ // save config
                // Limit num of saves
                organizer janitor;
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), jackettInfo.archivePath); // Cleanup

                std::cout << ANSI_COLOR_GREEN << "Synchronization of Jackett finished." << ANSI_COLOR_RESET << std::endl;
            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Jackett failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(std::string(argv[2]) == "prowlarr" || std::string(argv[2]) == "Prowlarr"){ // Prowlarr sync subparam
            // Create archive if it doesnt exists
            if(!std::filesystem::create_directories(prowlarrInfo.archivePath)){
                std::filesystem::create_directories(prowlarrInfo.archivePath);
            }

            if(create_save(prowlarrInfo.configPaths, "Prowlarr", prowlarrInfo.archivePath, exePath, logfile) == 1){
                // Limit num of saves
                organizer janitor;
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), prowlarrInfo.archivePath); // Cleanup

                std::cout << ANSI_COLOR_GREEN << "Synchronization of Prowlarr finished." << ANSI_COLOR_RESET << std::endl;

            }
            else{
                std::cout << ANSI_COLOR_RED << "Synchronization of Prowlarr failed." << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){  // qBittorrent sync subparam

            // Create archive if it doesnt exists
            if(!std::filesystem::create_directories(qbittorrentInfo.archivePath)){
                std::filesystem::create_directories(qbittorrentInfo.archivePath);
            }

            if(create_save(qbittorrentInfo.configPaths, "qBittorrent", qbittorrentInfo.archivePath, exePath, logfile) == 1){
                // Limit num of saves
                organizer janitor;
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), qbittorrentInfo.archivePath); // Cleanup

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
            unsigned jackettState = 0;
            if(rollback(jackettInfo.programName, jackettInfo.configPaths, exePath, logfile, failList, pt.get<unsigned>("recyclelimit")) == 1){
                jackettState = 1;
            }

            
            // @section Prowlarr
            unsigned prowlarrState = 0;
            if(rollback(prowlarrInfo.programName, prowlarrInfo.configPaths, exePath, logfile, failList, pt.get<unsigned>("recyclelimit")) == 1){
                prowlarrState = 1;
            }


            // @section qBittorrent
            unsigned qbitState = 0;
            if(rollback(qbittorrentInfo.programName, qbittorrentInfo.configPaths, exePath, logfile, failList, pt.get<unsigned>("recyclelimit")) == 1){
                qbitState = 1;
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
            
            analyzer anly(jackettInfo.configPaths, "Jackett", exePath, logfile); // Initialize class

            std::vector<std::string> jackettSaves;
            anly.get_all_saves(jackettSaves);


            if(argv[3] == NULL){ // default behavior. Use latest save. No 2subparam provided

                if(anly.is_archive_empty() == 1){ // Archive empty
                    std::cout << "Archive does not contain previous saves of Jackett." << std::endl;
                    std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
                }
                else{ // Archive not empty
                    synchronizer sync("Jackett", exePath, logfile); // Initialize class

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit"), jackettInfo.configPaths) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }

                }
            }


            else if(std::find(jackettSaves.begin(), jackettSaves.end(), std::string(argv[3])) != jackettSaves.end()){ // Specific save date 2subparam. Check if date is valid.

                synchronizer sync("Jackett", exePath, logfile); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("recyclelimit"), jackettInfo.configPaths) == 1){ // Restore from user defined save date<
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
            analyzer anly(prowlarrInfo.configPaths, prowlarrInfo.programName, exePath, logfile); // Initialize class

            std::vector<std::string> prowlarrSaves;
            anly.get_all_saves(prowlarrSaves);


            if(argv[3] == NULL){ // default behavior. Use latest save. No 2subparam provided

                if(anly.is_archive_empty() == 1){ // Archive empty
                    std::cout << "Archive does not contain previous saves of Prowlarr." << std::endl;
                    std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
                }
                else{ // Archive not empty
                    synchronizer sync(prowlarrInfo.programName, exePath, logfile); // Initialize class

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit"), prowlarrInfo.configPaths) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }


            else if(std::find(prowlarrSaves.begin(), prowlarrSaves.end(), std::string(argv[3])) != prowlarrSaves.end()){ // Specific save date 2subparam. Check if date is valid.

                synchronizer sync(prowlarrInfo.programName, exePath, logfile); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("recyclelimit"), prowlarrInfo.configPaths) == 1){ // Restore from user defined save date
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
            analyzer anly(qbittorrentInfo.configPaths, "qBittorrent", exePath, logfile); // Initialize class

            std::vector<std::string> qbittorrentSaves;
            anly.get_all_saves(qbittorrentSaves);


            if(argv[3] == NULL){ // default behavior. Use latest save. No 2subparam provided

                if(anly.is_archive_empty() == 1){ // Archive empty
                    std::cout << "Archive does not contain previous saves of Jackett." << std::endl;
                    std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
                }
                else{ // Archive not empty
                    synchronizer sync("qBittorrent", exePath, logfile); // Initialize class

                    if(sync.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit"), qbittorrentInfo.configPaths) == 1){ // Restore from newest save
                        std::cout << ANSI_COLOR_GREEN << "Rollback was successfull!" << ANSI_COLOR_RESET << std::endl;
                    }
                    else{
                        std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }


            else if(std::find(qbittorrentSaves.begin(), qbittorrentSaves.end(), std::string(argv[3])) != qbittorrentSaves.end()){ // Specific save date 2subparam. Check if date is valid.

                synchronizer sync("qBittorrent", exePath, logfile); // Initialize class

                if(sync.restore_config(std::string(argv[3]), pt.get<int>("recyclelimit"), qbittorrentInfo.configPaths) == 1){ // Restore from user defined save date
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


            for(const auto& app : PC.get_support_list()){ // Fetch status

                
                analyzer anly(PC.get_ProgramInfo(app).configPaths, app, exePath, logfile); // Init class

                if(anly.is_archive_empty() == 1){ // Check for previous save
                    neverSaveList.insert(app);
                }
                else{ // Save exists
                    
                    if(anly.is_identical() == 1){
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

            analyzer anly(jackettInfo.configPaths, "Jackett", exePath, logfile); // Init class
            
            std::cout << "Program: Jackett" << std::endl;
            std::cout << "Last Save: " << anly.get_newest_backup_path() << std::endl;
            
            
            if(anly.is_archive_empty() == 1){ // Check if save exists
                std::cout << "Error: No previous save exists for Jackett." << std::endl;
            }
            else{ // Save exists

                if(anly.is_identical() == 1){ // Compare config
                    std::cout << ANSI_COLOR_GREEN << "Config is up to date!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_RED << "Config is out of sync!" << ANSI_COLOR_RESET << std::endl;
                }
            }
        }


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr subparam

            analyzer anly(prowlarrInfo.configPaths, "Prowlarr", exePath, logfile); // Init class
            
            std::cout << "Program: Prowlarr" << std::endl;
            std::cout << "Last Save: " << anly.get_newest_backup_path() << std::endl;
            
            
            if(anly.is_archive_empty() == 1){ // Check if save exists
                std::cout << "Error: No previous save exists for Prowlarr." << std::endl;
            }
            else{ // Save exists

                if(anly.is_identical() == 1){ // Compare config
                    std::cout << ANSI_COLOR_GREEN << "Config is up to date!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_RED << "Config is out of sync!" << ANSI_COLOR_RESET << std::endl;
                }
            }
        }


        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent subparam

            analyzer anly(qbittorrentInfo.configPaths, "qBittorrent", exePath, logfile); // Init class
            
            std::cout << "Program: qBittorrent" << std::endl;
            std::cout << "Last Save: " << anly.get_newest_backup_path() << std::endl;
            
            
            if(anly.is_archive_empty() == 1){ // Check if save exists
                std::cout << "Error: No previous save exists for qBittorrent." << std::endl;
            }
            else{ // Save exists

                if(anly.is_identical() == 1){ // Compare config
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

            analyzer anly(jackettInfo.configPaths, "Jackett", exePath, logfile); // Init class

            
            if(anly.is_archive_empty() == 1) {
                std::cout << "Archive does not contain previous saves of Jackett." << std::endl;
                std::cout << "Use 'cfgs --sync jackett' to create one." << std::endl;
            }
            else{
                std::vector<std::string> saveList;
                anly.get_all_saves(saveList);
                anly.sortby_filename(saveList);

                // Fetch Index of backups
                auto IX = anly.get_Index("Jackett", exePath);
    
                std::cout << ANSI_COLOR_222 << "Showing Jackett:" << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_166 << "Snapshots: " << ANSI_COLOR_RESET << std::endl;
                unsigned i = 1;
                for(const auto& save : saveList){ // Show all saves
                    std::cout << ANSI_COLOR_146 << i << ". " << save << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                
                // Show Restore Snapshots:
                std::cout << ANSI_COLOR_166 << "Restore Snapshots: " << ANSI_COLOR_RESET << std::endl;
                i = 0;
                for(const auto& pair : IX.time_uuid){
                    std::cout << ANSI_COLOR_151 << i << ". " << synchronizer::timestamp_to_string(pair.first) << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                std::cout << "Found " << saveList.size() << " saves and " << i << " Restore Snapshots" << std::endl;
            }
        }


        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // Prowlarr subparam
            analyzer anly(prowlarrInfo.configPaths, "Prowlarr", exePath, logfile); // Init class
            
            if(anly.is_archive_empty() == 1) {
                std::cout << "Archive does not contain previous saves of Prowlarr." << std::endl;
                std::cout << "Use 'cfgs --sync prowlarr' to create one." << std::endl;
            }
            else{
                std::vector<std::string> saveList;
                anly.get_all_saves(saveList);
                anly.sortby_filename(saveList);

                // Fetch Index of backups
                auto IX = anly.get_Index("Prowlarr", exePath);
    
                std::cout << ANSI_COLOR_222 << "Showing Prowlarr:" << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_166 << "Snapshots: " << ANSI_COLOR_RESET << std::endl;
                unsigned i = 1;
                for(const auto& save : saveList){ // Show all saves
                    std::cout << ANSI_COLOR_146 << i << ". " << save << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                
                // Show Restore Snapshots:
                std::cout << ANSI_COLOR_166 << "Restore Snapshots: " << ANSI_COLOR_RESET << std::endl;
                i = 0;
                for(const auto& pair : IX.time_uuid){
                    std::cout << ANSI_COLOR_151 << i << ". " << synchronizer::timestamp_to_string(pair.first) << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                std::cout << "Found " << saveList.size() << " saves and " << i << " Restore Snapshots" << std::endl;
            }
        }


        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // qBittorrent subparam

            analyzer anly(qbittorrentInfo.configPaths, "qBittorrent", exePath, logfile); // Init class

            
            if(anly.is_archive_empty() == 1) { // Archive empty
                std::cout << "Archive does not contain previous saves of qBittorrent." << std::endl;
                std::cout << "Use 'cfgs --sync qbittorrent' to create a save." << std::endl;
            }
            else{ // Archive not empty
                std::vector<std::string> saveList;
                anly.get_all_saves(saveList);
                anly.sortby_filename(saveList);

                // Fetch Index of backups
                auto IX = anly.get_Index("Prowlarr", exePath);
    
                std::cout << ANSI_COLOR_222 << "Showing qBittorrent:" << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_166 << "Snapshots: " << ANSI_COLOR_RESET << std::endl;
                unsigned i = 1;
                for(const auto& save : saveList){ // Show all saves
                    std::cout << ANSI_COLOR_146 << i << ". " << save << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                
                // Show Restore Snapshots:
                std::cout << ANSI_COLOR_166 << "Restore Snapshots: " << ANSI_COLOR_RESET << std::endl;
                i = 0;
                for(const auto& pair : IX.time_uuid){
                    std::cout << ANSI_COLOR_151 << i << ". " << synchronizer::timestamp_to_string(pair.first) << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                std::cout << "Found " << saveList.size() << " saves and " << i << " Restore Snapshots" << std::endl;
            }
        }


        else{
            std::cerr << "Fatal: invalid argument. See 'cfgs --help'.\n";
        }
    }


    else if(std::string(argv[1]) == "list"){ // 'list' param

        std::cout << "Supported Programs: " << std::endl;

        unsigned int i = 1;
        for(const auto& app : PC.get_support_list()){
            std::cout << i << ". " << app << std::endl;
            i++;
        }
    }


    else if(std::string(argv[1]) == "verify"){ // 'verify' param
        // TODO
    }


    else if(std::string(argv[1]) == "revert"){ // 'revert' param
        if(argv[2] == NULL){
            std::cerr << ANSI_COLOR_RED << "Fatal: missing argument or value. See 'cfgs --help'." << ANSI_COLOR_RESET << std::endl;
        }

        
        else if(std::string(argv[2]) == "--all"){ // '--all' subparam
            std::cout << ANSI_COLOR_166 << "Reverting all restores:" << ANSI_COLOR_RESET << std::endl;

            for(const auto& app : ProgramConfig::get_support_list()){
                revertRestore(app, exePath, logfile, std::nullopt);
            }
        }
        
        else if(std::string(argv[2]) == "Jackett" || std::string(argv[2]) == "jackett"){ // 'Jackett' subparam
            if(argv[3] != NULL){
                // Create optional
                std::string input = std::string(argv[3]);
                std::optional<std::reference_wrapper<std::string>> userInputRef = std::ref(input);
                // Try to revert
                revertRestore("Jackett", exePath, logfile, userInputRef);
            }
            else{
                revertRestore("Jackett", exePath, logfile);
            }
        }

        else if(std::string(argv[2]) == "Prowlarr" || std::string(argv[2]) == "prowlarr"){ // 'Prowlarr' subparam
            if(argv[3] != NULL){
                // Create optional
                std::string input = std::string(argv[3]);
                std::optional<std::reference_wrapper<std::string>> userInputRef = std::ref(input);
                // Try to revert
                revertRestore("Prowlarr", exePath, logfile, userInputRef);
            }
            else{
                revertRestore("Prowlarr", exePath, logfile);
            }
        }
        
        else if(std::string(argv[2]) == "qBittorrent" || std::string(argv[2]) == "qbittorrent"){ // 'qBittorrent' subparam
            if(argv[3] != NULL){
                // Create optional
                std::string input = std::string(argv[3]);
                std::optional<std::reference_wrapper<std::string>> userInputRef = std::ref(input);
                // Try to revert
                revertRestore("qBittorrent", exePath, logfile, userInputRef);
            }
            else{
                revertRestore("qBittorrent", exePath, logfile);
            }
        }
        
        else{
            std::cerr << "Fatal: invalid argument. See 'cfgs --settings'." << std::endl;
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
            std::cout << "(" << pt.get<int>("savelimit") << ") ==> (" << DS.savelimit << ")" << std::endl;

            pt.put("savelimit", DS.savelimit); // Reset

            boost::property_tree::write_json(settingsPath, pt); // Update config file
        }

        
        else if(std::string(argv[2]) == "recyclelimit" || std::string(argv[2]) == "--recyclelimit"){ // 'recyclelimit' subparam

            std::cout << "RecycleLimit reset to default." << std::endl;
            std::cout << "(" << pt.get<int>("recyclelimit") << ") ==> (" << DS.recyclelimit << ")" << std::endl;

            pt.put("recyclelimit", DS.recyclelimit); // Reset

            boost::property_tree::write_json(settingsPath, pt); // Update config file
        }

        
        else if(std::string(argv[2]) == "task" || std::string(argv[2]) == "--task"){ // 'task' subparam

            std::cout << "Task reset to default." << std::endl;
            std::cout << "(" << pt.get<bool>("task") << ") ==> (" << DS.task << ")" << std::endl;

            pt.put("task", DS.task); // Reset
            boost::property_tree::write_json(settingsPath, pt); // Update config file
            

            if(DS.task == false && task::exists(taskName) == 1){ // Defaultsetting for 'task' is false and task exists
                if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);} // Remove the task
            }

            else if(DS.task == true && task::exists(taskName) == 0){ // Defaultsetting for 'task' is true and task doesnt yet exist.
                task::createtask(taskName, batPath, DS.taskfrequency); // Create the task
            }
            else{std::cerr << "File: [" << __FILE__ << "] Line: [" << __LINE__ << "] This should never happen!\n";}
        }


        else if(std::string(argv[2]) == "taskfrequency" || std::string(argv[2]) == "--taskfrequency"){ // 'taskfrequency' subparam

            std::cout << "TaskFrequency reset to default." << std::endl;
            std::cout << "(" << pt.get<std::string>("taskfrequency") << ") ==> (" << DS.taskfrequency << ")" << std::endl;

            pt.put("taskfrequency", DS.taskfrequency); // Reset
            boost::property_tree::write_json(settingsPath, pt); // Update config file


            if(task::exists(taskName) == 1){ // Task exists.
                if(task::removetask(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);}
                task::createtask(taskName, batPath, DS.taskfrequency); // Create task to apply the reset taskfrequeny
            }
            else if(pt.get<bool>("task") == true){ // Task does not exist but tasksetting is on. 
                task::createtask(taskName, batPath, DS.taskfrequency); // Create task
            }
            else{std::cerr << "File: [" << __FILE__ << "] Line: [" << __LINE__ << "] This should never happen!\n";}
        }
        

        else if(std::string(argv[2]) == "--all"){ // '--all' subparam

            pt.put("savelimit", DS.savelimit);
            pt.put("recyclelimit", DS.recyclelimit);
            pt.put("task", DS.task);
            pt.put("taskfrequency", DS.taskfrequency);

            boost::property_tree::write_json(settingsPath, pt); // Update config file
            

            if(DS.task == true && !task::exists(taskName)){ // Task default is 'true' and task doesnt exist.

                task::createtask(taskName, batPath, DS.taskfrequency); // Create the task
            }

            else if(DS.task == false && task::exists(taskName)){ // Task default is 'false' and task exists.

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