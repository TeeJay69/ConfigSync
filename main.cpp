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
#include <thread>
#include <TlHelp32.h>
#include <ranges>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>
#include <boost\filesystem\operations.hpp>
#include <boost\filesystem\path.hpp>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost/dll.hpp>
#include "synchronizer.hpp"


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

int argcmp(char** argv, int* argc, const char* cmp){
    for(int i = 0; i < *argc; i++){
        if(strcmp(argv[i], cmp) == 0){
            return 1;
        }
    }

    return 0;
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

                    return 0;
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
            else{
                recurse_remove(dateDir); // Remove the corrupt save, it has no hashbase
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
    }
    else{   
        std::cerr << "Verification of " << program << " location failed." << std::endl;
        return 0;
    }

    return 1;
}

int handleSyncOption(char** argv, const boost::property_tree::ptree& pt, const std::string& exePath, std::ofstream& logfile){
    ProgramConfig PC(exePath);
    const auto& supportedList = PC.get_support_list();

    if(argv[2] == NULL){ // No subparam provided
            std::cout << "Fatal: Missing program or restore argument." << std::endl;
    }
    else if(std::string(argv[2]) == "--all"){ // Create a save of all supported, installed programs. No subparam provided
        std::cout << ANSI_COLOR_161 << "Synchronizing all programs:" << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_222 << "Fetching paths..." << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_222 << "Checking archive..." << ANSI_COLOR_RESET << std::endl;
        
        std::vector<std::string> syncedList;
        std::cout << ANSI_COLOR_222 << "Starting synchronization..." << ANSI_COLOR_RESET << std::endl;
        organizer janitor; // Initialize class

        for(const auto& app : PC.get_unique_support_list()){
            const auto& pInfo = PC.get_ProgramInfo(app);
            // Create the archive dir if it does not exist yet
            if(!std::filesystem::exists(pInfo.archivePath)){
                std::filesystem::create_directories(pInfo.archivePath);
            }

            if(create_save(pInfo.configPaths, pInfo.programName, pInfo.archivePath, exePath, logfile) == 1){
                syncedList.push_back(pInfo.programName);
                janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), pInfo.archivePath); // Cleanup
            }
        }

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
    else if(std::find(supportedList.begin(), supportedList.end(), std::string(argv[2])) != supportedList.end()){ // Jackett sync subparam
        const auto &pInfo = PC.get_ProgramInfo(std::string(argv[2]));

         // Create archive if it doesnt exist
        if(!std::filesystem::exists(pInfo.archivePath)){
            std::filesystem::create_directories(pInfo.archivePath);
        }
        
        if(create_save(pInfo.configPaths, pInfo.programName, pInfo.archivePath, exePath, logfile) == 1){ // snapshot config
            // Limit num of snaps
            organizer janitor;
            janitor.limit_enforcer_configarchive(pt.get<int>("savelimit"), pInfo.archivePath);
            std::cout << ANSI_COLOR_GREEN << "Synchronization of " << pInfo.programName << " finished." << ANSI_COLOR_RESET << std::endl;
        }
        else{
            std::cout << ANSI_COLOR_RED << "Synchronization of " << pInfo.programName << " failed." << ANSI_COLOR_RESET << std::endl;
            return 0;
        }

    }
    else{ // Invalid subparam provided
        std::cerr << "Fatal: '" << argv[2] << "' is not a ConfigSync command. See 'cfgs --help'." << std::endl;
        return 0;
    }

    return 1;
}

int handleRestoreOption(char** argv, const boost::property_tree::ptree& pt, const std::string& exePath, std::ofstream& logfile){
    ProgramConfig PC(exePath);
    const auto &supportList = PC.get_support_list();

    if(argv[2] == NULL){ // Missing argument
        std::cout << "Fatal: Missing argument or operand." << std::endl;
        std::cout << "For usage see 'cfgs --help'" << std::endl;
    }
    else if(std::string(argv[2]) == "all" || std::string(argv[2]) == "--all"){ // '--all' operand
        if(std::string(argv[3]) == "--force"){ // '--force' sub operand
            std::cout << ANSI_COLOR_161 << "Forced restoring all supported programs with latest save..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Killing all running instances..." << ANSI_COLOR_RESET << std::endl;
            
            Process proc;
            for(const auto& app : ProgramConfig::get_unique_support_list()){
                for(const auto& process : PC.get_ProgramInfo(app).processNames){
                    proc.killProcess(process.data());
                }
            }
            std::cout << ANSI_COLOR_222 << "Kill streak finished..." << ANSI_COLOR_RESET << std::endl;
        }
        else{
            std::cout << ANSI_COLOR_YELLOW << "Restoring all supported programs with latest save..." << ANSI_COLOR_RESET << std::endl;
        }
        
        std::vector<std::string> failList;
        
        for(const auto& app : PC.get_unique_support_list()){

            const auto& pInfo = PC.get_ProgramInfo(app);
            analyzer anly(pInfo.configPaths, pInfo.programName, exePath, logfile); // Initialize class
            synchronizer syncProgram(pInfo.programName, exePath, logfile); // Initialize class

            if(anly.is_archive_empty() == 1){ // Archive is empty
                std::cout << "Skipping " << pInfo.programName << ", not synced..." << std::endl;
                failList.push_back(pInfo.programName);
            }
            else{ // Archive not empty

                if(syncProgram.restore_config(anly.get_newest_backup_path(), pt.get<unsigned>("recyclelimit"), pInfo.configPaths) == 1){ // Restore from newest save
                    std::cout << ANSI_COLOR_GREEN << pInfo.programName << " restore successfull!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << pInfo.programName << " restore failed." << ANSI_COLOR_RESET << std::endl;
                    failList.push_back(pInfo.programName);
                }
            }
        }
        

        // User info
        if(failList.empty()){ // All success
            std::cout << ANSI_COLOR_GREEN << "Restore complete!" << ANSI_COLOR_RESET << std::endl; 
        }
        else{ // Partial success
            std::cout << ANSI_COLOR_YELLOW << "Restore finished!." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_RED << "Failed to restore:" << ANSI_COLOR_RESET << std::endl;
            
            int i = 1;
            for(const auto& app : failList){ // Display failed rollbacks
                std::cout << ANSI_COLOR_RED << i << ".  " << app << ANSI_COLOR_RESET << std::endl;
                i++;
            }
        }
    }
    else if(std::find(supportList.begin(), supportList.end(), std::string(argv[2])) != supportList.end()){ // Single restore

        const auto &programInfo = PC.get_ProgramInfo(std::string(argv[2]));
        
        
        analyzer anly(programInfo.configPaths, programInfo.programName, exePath, logfile);
        std::vector<std::string> programSaves;
        anly.get_all_saves(programSaves);

        if(argv[3] == NULL){ // default behavior. Use latest save. No 2sub option provided
            std::cout << ANSI_COLOR_161 << "Restoring " << programInfo.programName << " using latest snapshot:" << ANSI_COLOR_RESET << std::endl;

            if(anly.is_archive_empty() == 1){
                std::cout << ANSI_COLOR_YELLOW << "Archive does not contain previous snapshots of Jackett." << ANSI_COLOR_RESET << std::endl;
                std::cout << "Use 'cfgs sync '" << programInfo.programName << "' to create one." << std::endl;
                return 0;
            }
            else{
                synchronizer synchro(programInfo.programName, exePath, logfile);
                
                if(synchro.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit"), programInfo.configPaths) == 1){
                    std::cout << ANSI_COLOR_GREEN << "Restore of config was successfull!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    return 0;
                }
            }
        }


        else if(std::string(argv[3]) == "--force"){ // '--force' 2sub operand
            std::cout << ANSI_COLOR_161 << "Forced restore of " << programInfo.programName << " using latest snapshot..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Killing all running instances of " << programInfo.programName << "..." << ANSI_COLOR_RESET << std::endl;

            Process proc;
            for(const auto& process : programInfo.processNames){
                proc.killProcess(process.data());
            }
            std::cout << ANSI_COLOR_222 << "Kill streak! finished..." << ANSI_COLOR_RESET << std::endl;
            
            if(anly.is_archive_empty() == 1){
                std::cout << ANSI_COLOR_YELLOW << "Archive does not contain previous snapshots of Jackett." << ANSI_COLOR_RESET << std::endl;
                std::cout << "Use 'cfgs --sync '" << programInfo.programName << "' to create one." << std::endl;
                return 0;
            }
            else{
                synchronizer synchro(programInfo.programName, exePath, logfile);
                if(synchro.restore_config(anly.get_newest_backup_path(), pt.get<int>("recyclelimit"), programInfo.configPaths) == 1){
                    std::cout << ANSI_COLOR_GREEN << "Restore of config was successfull!" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                    return 0;
                }
            }
        }

        else if(std::find(programSaves.begin(), programSaves.end(), std::string(argv[3])) != programSaves.end()){ // Specific save date 2sub option. Check if date is valid.
            if(std::string(argv[4]) == "--force"){ // Check for '--force' 3sub operand
                std::cout << ANSI_COLOR_161 << "Forced restore of " << programInfo.programName << " using specified snapshot..." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_222 << "Killing all running instances of " << programInfo.programName << "..." << ANSI_COLOR_RESET << std::endl;

                Process proc;
                for(const auto& process : programInfo.processNames){
                    proc.killProcess(process.data());
                }
                std::cout << ANSI_COLOR_222 << "Kill streak! finished..." << ANSI_COLOR_RESET << std::endl;
            }
            else{
                std::cout << ANSI_COLOR_161 << "Restore of " << programInfo.programName << " using specified snapshot..." << ANSI_COLOR_RESET << std::endl;
            }

            synchronizer synchro(programInfo.programName, exePath, logfile); // Init class
            if(synchro.restore_config(std::string(argv[3]), pt.get<int>("recyclelimit"), programInfo.configPaths) == 1){
                std::cout << ANSI_COLOR_GREEN << "Restore of config was successfull!" << ANSI_COLOR_RESET << std::endl;
            }
            else{
                std::cerr << ANSI_COLOR_RED << "Failed to restore config." << ANSI_COLOR_RESET << std::endl;
                return 0;
            }
        }
        else{ // Invalid 2subparameter (not null + not valid)
            std::cerr << "Fatal: '" << argv[3] << "' is not a valid date." << std::endl;
            std::cout << "To view all save dates use 'cfgs show " << programInfo.programName << "'" << std::endl;
            return 0;
        }
    }
    else{
        std::cerr << "Fatal: invalid argument. See 'cfgs --help'.\n";
        return 0;
    }

    return 1;
}

int handleStatusOption(char** argv, const boost::property_tree::ptree& pt, const std::string& exePath, std::ofstream& logfile){
    ProgramConfig PC(exePath);
    const auto& supportList = PC.get_support_list();
    
    if(argv[2] == NULL){ // default 

        std::cout << ANSI_COLOR_166 << "Status:" << ANSI_COLOR_RESET << std::endl;

        std::set<std::string> neverSaveList;
        std::set<std::string> outofSyncList;
        std::set<std::string> inSyncList;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << ANSI_COLOR_222 << "Fetching programs..." << ANSI_COLOR_RESET << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << ANSI_COLOR_222 << "Calculating hashes..." << ANSI_COLOR_RESET << std::endl;

        for(const auto& app : PC.get_unique_support_list()){
            analyzer anly(PC.get_ProgramInfo(app).configPaths, app, exePath, logfile); // Init class

            if(anly.is_archive_empty() == 1){
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

    else if(std::find(supportList.begin(), supportList.end(), std::string(argv[2])) != supportList.end()){
        const auto& pInfo = PC.get_ProgramInfo(std::string(argv[2]));

        analyzer anly(pInfo.configPaths, pInfo.programName, exePath, logfile); // Init class
            
        std::cout << ANSI_COLOR_161 << "Status " << pInfo.programName << ":" << ANSI_COLOR_RESET << std::endl;
        
        
        if(anly.is_archive_empty() == 1){ // Check if save exists
            std::cerr << ANSI_COLOR_RED << "Error: No previous snapshot exists for" << pInfo.programName << " ." << ANSI_COLOR_RESET << std::endl;
            return 0;
        }
        else{ // Save exists
            std::cout << ANSI_COLOR_222 << "Last Save: " << std::filesystem::path(anly.get_newest_backup_path()).filename() << ANSI_COLOR_RESET << std::endl;

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

    return 1;
}

int handleShowOption(char** argv, const boost::property_tree::ptree& pt, const std::string& exePath, std::ofstream& logfile){

    ProgramConfig PC(exePath);
    const auto &supportedList = PC.get_support_list();


    if(argv[2] == NULL){ // default. No subparam provided
        std::cerr << "Fatal: missing program argument." << std::endl;
        std::cout << "See 'cfgs --help'." << std::endl;
        return 0;
    }

        
    else if(std::find(supportedList.begin(), supportedList.end(), std::string(argv[2])) != supportedList.end()){ // specific program sub option

        auto const &pInfo = PC.get_ProgramInfo(std::string(argv[2]));

        analyzer anly(pInfo.configPaths, pInfo.programName, exePath, logfile); // Init class

        
        if(anly.is_archive_empty() == 1) {
            std::cout << "Archive does not contain previous snapshots of " << pInfo.programName << "." << std::endl;
            std::cout << "Use 'cfgs sync " << pInfo.programName << "' to create one." << std::endl;
            return 0;
        }
        else{
            std::vector<std::string> saveList;
            anly.get_all_saves(saveList);
            anly.sortby_filename(saveList);

            // Fetch Index of backups
            const auto &IX = anly.get_Index(pInfo.programName, exePath);

            std::cout << ANSI_COLOR_222 << "Showing " << pInfo.programName << ":" << ANSI_COLOR_RESET << std::endl;
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
        return 0;
    }

    return 1;
}

std::string* matchDate(const std::string& userInput, const std::string& programName, const Index& IX, synchronizer& syncro, std::ofstream& logfile){
    std::string match;
    unsigned hit = 0;
    for(const auto& pair : IX.time_uuid){
        // Convert unsigned long long to string and match for userInput
        if(synchronizer::timestamp_to_string(pair.first).find(userInput) != std::string::npos){
            hit++;
            match = pair.second; // we want uuid
        }
        else if(pair.second.find(userInput) != std::string::npos){
            hit++;
            match = pair.second; // we want uuid
        }
    }

    logfile << logs::ms("matchDate userInput hits: ") << hit << "Program:" << programName << "\n";

    if(hit > 1){
        // More than 1 hit, can't allow that...
        logfile << ANSI_COLOR_RED << logs::ms("Fatal: provided date or uuid was not unique.") << ANSI_COLOR_RESET << std::endl;
        std::cerr << ANSI_COLOR_RED << "Fatal: provided date or uuid was not unique." << ANSI_COLOR_RESET << std::endl;
        return {};
    }
    else if(hit == 0){
        // No match, cant have that either...
        logfile << ANSI_COLOR_RED << logs::ms("Fatal: provided date or uuid is invalid.") << ANSI_COLOR_RESET << std::endl;
        std::cerr << ANSI_COLOR_RED << "Fatal: provided date or uuid is invalid." << ANSI_COLOR_RESET << std::endl;
        return {};
    }

    return &match;
}

int handleRevertOption(char** argv, const boost::property_tree::ptree& pt, const std::string& exePath, std::ofstream& logfile){

    ProgramConfig PC(exePath);
    const auto &supportedList = PC.get_support_list();
    
    if(argv[2] == NULL){
        std::cerr << ANSI_COLOR_RED << "Fatal: missing argument or value. See 'cfgs --help'." << ANSI_COLOR_RESET << std::endl;
    }
    
    else if(std::string(argv[2]) == "--all"){ // '--all' sub operand

        if(std::string(argv[3]) == "--force"){ // '--force' sub sub operand
            std::cout << ANSI_COLOR_161 << "Forced revert of all programs:" << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Killing all running instances..." << ANSI_COLOR_RESET << std::endl;

            Process proc;
            for(const auto& app : PC.get_unique_support_list()){
                for(const auto& process : PC.get_ProgramInfo(app).processNames){
                    proc.killProcess(process.data());
                }
            }

            std::cout << ANSI_COLOR_222 << "Kill streak finished..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_166 << "Reverting all restores:" << ANSI_COLOR_RESET << std::endl;
        }
        else{
            std::cout << ANSI_COLOR_166 << "Reverting all restores:" << ANSI_COLOR_RESET << std::endl;
        }

        std::vector<std::string> revertSuccessList;
        std::vector<std::string> revertFailList;
        std::vector<std::string> neverRestoredList; // no restore made ergo no revert possible

        for(const auto& app : PC.get_unique_support_list()){

            if(analyzer::has_backup(app, exePath)){
                // no specific date
                std::cout << ANSI_COLOR_222 << "Reverting last " << app << " restore:" << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_222 << "Fetching index..." << ANSI_COLOR_RESET << std::endl;
                // Get index
                Index IX = analyzer::get_Index(app, exePath);

                synchronizer sync(exePath, app, logfile);
                
                std::cout << ANSI_COLOR_222 << "Starting revert..." << ANSI_COLOR_RESET << std::endl;
                // Try to undo last restore
                if(sync.revert_restore(IX.time_uuid.back().second) != 1){
                    revertFailList.push_back(app);
                }
                else{
                    revertSuccessList.push_back(app);
                }


                std::cout << ANSI_COLOR_222 << "Revert finished, cleaning up..." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_GREEN << "Revert was successfull!" << ANSI_COLOR_RESET << std::endl;
            }
            else{
                neverRestoredList.push_back(app);
            }
        }

        std::cout << ANSI_COLOR_222 << "Preparing Summary..." << ANSI_COLOR_RESET << std::endl;
        if(!revertSuccessList.empty()){
            std::cout << ANSI_COLOR_GREEN << "Successfull:" << ANSI_COLOR_RESET << std::endl;
            unsigned i = 1;
            for(const auto& elem : revertSuccessList){
                std::cout << ANSI_COLOR_GREEN << i << ". " << elem << ANSI_COLOR_RESET << std::endl;
                i++;
            }
        }
        
        if(!neverRestoredList.empty()){
            std::cout << ANSI_COLOR_YELLOW << "Never restored:" << ANSI_COLOR_RESET << std::endl;
            unsigned ii = 1;
            for(const auto& elem : neverRestoredList){
                std::cout << ANSI_COLOR_YELLOW << ii << ". " << elem << ANSI_COLOR_RESET << std::endl;
                ii++;
            }
        }

        if(!revertFailList.empty()){
            std::cout << ANSI_COLOR_RED << "Failed:" << ANSI_COLOR_RESET << std::endl;
            unsigned iii = 1;
            for(const auto& elem : revertFailList){
                std::cout << ANSI_COLOR_RED << iii << ". " << elem << ANSI_COLOR_RESET << std::endl;
                iii++;
            }

            std::cout << ANSI_COLOR_RED << "Warning! At least one program restore could not be reverted. Please verify that none of that programs components are missing or corrupted!" << std::endl;
            logfile << logs::ms("Error: Failed to revert last restore.\n");
        }
    }
    else if(std::find(supportedList.begin(), supportedList.end(), std::string(argv[2])) != supportedList.end()){ // specific program sub option
        const auto &pInfo = PC.get_ProgramInfo(std::string(argv[2]));
        // Get Index
        Index IX = analyzer::get_Index(pInfo.programName, exePath);
        synchronizer sync(exePath, pInfo.programName, logfile);
        
        if(argv[3] == NULL){ // no specific restore date 
            std::cout << ANSI_COLOR_161 << "Revert of last " << pInfo.programName << " restore:" << ANSI_COLOR_RESET << std::endl; 
            
            if(analyzer::has_backup(pInfo.programName, exePath) != 1){
                std::cout << ANSI_COLOR_RED << "Fatal: No previous restore of " << pInfo.programName << " exists to revert" << ANSI_COLOR_RESET << std::endl;
                return 0;
            }

            // Try to undo last restore
            if(sync.revert_restore(IX.time_uuid.back().second) != 1){
                std::cout << ANSI_COLOR_RED << "Failed to revert the last restore!\nPlease verify that none of that programs components are missing or corrupted!" << ANSI_COLOR_RESET << std::endl;
                return 0;
            }
            else{
                // std::cout << ANSI_COLOR_222 << "Revert finished, cleaning up..." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_GREEN << "Revert was successfull!" << ANSI_COLOR_RESET << std::endl;
            }
        }

        else if(std::string(argv[3]) == "--force"){ // '--force' operand
            std::cout << ANSI_COLOR_161 << "Forced revert of last " << pInfo.programName << " restore:" << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Killing all running instances..." << ANSI_COLOR_RESET << std::endl;

            Process proc;
            for(const auto& process : pInfo.processNames){
                    proc.killProcess(process.data());
            }
            std::cout << ANSI_COLOR_222 << "Kill streak! finished..." << ANSI_COLOR_RESET << std::endl;
            
            if(analyzer::has_backup(pInfo.programName, exePath) != 1){
                std::cout << ANSI_COLOR_RED << "Fatal: No previous restore of " << pInfo.programName << " exists to revert" << ANSI_COLOR_RESET << std::endl;
                return 0;
            }
            // Try to undo last restore
            if(sync.revert_restore(IX.time_uuid.back().second) != 1){
                std::cout << ANSI_COLOR_RED << "Failed to revert the last restore!\nPlease verify that none of that programs components are missing or corrupted!" << ANSI_COLOR_RESET << std::endl;
                return 0;
            }
            else{
                // std::cout << ANSI_COLOR_222 << "Revert finished, cleaning up..." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_GREEN << "Revert was successfull!" << ANSI_COLOR_RESET << std::endl;
            }
        }

        else{ // Specific restore specified
            if(std::string(argv[4]) == "--force"){ // '--force' operand
                std::cout << ANSI_COLOR_166 << "Forced revert of specified restore from " << pInfo.programName << " :" << ANSI_COLOR_RESET << std::endl;
                Process proc;
                for(const auto& process : pInfo.processNames){
                        proc.killProcess(process.data());
                }
                std::cout << ANSI_COLOR_222 << "Kill streak! finished..." << ANSI_COLOR_RESET << std::endl;
            }
            else{
                std::cout << ANSI_COLOR_166 << "Reverting specified restore of " << pInfo.programName << " :" << ANSI_COLOR_RESET << std::endl;
            }

            std::cout << ANSI_COLOR_222 << "Verifying provided date or UUID..." << ANSI_COLOR_RESET << std::endl;
            std::string *match = matchDate(std::string(argv[3]), pInfo.programName, IX, sync, logfile);
            if(!match->empty()){
                std::cout << ANSI_COLOR_222 << "Found a unique match..." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_222 << "Starting revert..." << ANSI_COLOR_RESET << std::endl;
                // Try to undo last restore wit the match uuid
                if(!sync.revert_restore(*match) != 1){
                    std::cerr << ANSI_COLOR_RED << "Error: Failed to revert specified restore.\n";
                    logfile << logs::ms("Error: Failed to revert specified restore.\n");
                    throw cfgsexcept("Please verify that none of your selected programs components are missing or corrupted");
                    return 0;
                }

                std::cout << ANSI_COLOR_222 << "Revert finished, cleaning up..." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_GREEN << "Revert was successfull!" << ANSI_COLOR_RESET << std::endl;
            }
            else{
                return 0;
            }
        }
    }
    else{
        std::cerr << "Fatal: invalid argument. See 'cfgs --settings'." << std::endl;
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

            const std::string com = "cmd /c \"schtasks /delete /tn \"" + taskname + "\" /F >NUL 2>&1\"";

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
        const int savelimit = 3;
        const int recyclelimit = 3;
        const bool task = false;
        const std::string taskfrequency = "daily,1";
};


int main(int argc, char* argv[]){   


    // Catches exit signal Ctrl+C
    std::signal(SIGINT, exitSignalHandler);
    // Enables colors in windows command processor
    enableColors();
    
    // Get location of program
    const std::string exePath = boost::dll::program_location().parent_path().string();
    const std::string settingsPath = exePath + "\\settings.json";
    const std::string batPath = exePath + "\\cfgs.bat";
    const std::string taskName = "ConfigSyncTask";
    std::ofstream logfile = logs::session_log(exePath, 50); // Create logfileile

    // Universal operands:
    int verbose = 0;
    
    // Classes
    defaultsetting DS;
    ProgramConfig PC(exePath);

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

    if(argcmp(argv, &argc, "--verbose") == 1 || argcmp(argv, &argc, "-v") == 1){
        verbose = 1;
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
        std::cout << "revert --all                  Revert restore of all programs.\n";
        std::cout << "status [PROGRAM]              Display synchronization state. (Default: all)\n";
        std::cout << "show [PROGRAM]                List all existing snapshots of a program\n";
        std::cout << "list --supported              Show all supported programs.\n";
        std::cout << "settings                      Show settings.\n";
        std::cout << "settings --json               Show settings (JSON).\n";
        std::cout << "settings.reset [SETTING]      Reset one or multiple settings.\n";
        std::cout << "settings.reset --all          Reset all settings to default.\n";
        std::cout << "settings --help               Show detailed settings info.\n";
        std::cout << "--help                        Display help message.\n";
        std::cout << "--version                     Display version and copyright disclaimer.\n\n";
        std::cout << "Operands:\n";
        std::cout << "[...] --force                 Kills running instances before, to avoid errors.\n";
        std::exit(EXIT_SUCCESS);
    }

    else if(std::string(argv[1]) == "sync"){ // Sync param
        handleSyncOption(argv, pt, exePath, logfile);
    }


    else if(std::string(argv[1]) == "restore" || std::string(argv[1]) == "--restore"){ // Restore param
        handleRestoreOption(argv, pt, exePath, logfile);
    }


    else if(std::string(argv[1]) == "status"){ // 'status' parag
        handleStatusOption(argv, pt, exePath, logfile);
    }


    else if(std::string(argv[1]) == "show" || std::string(argv[1]) == "--show"){ // 'show' param
        handleShowOption(argv, pt, exePath, logfile);
    }


    else if(std::string(argv[1]) == "list"){ // 'list' param

        std::cout << "Supported Programs: " << std::endl;

        unsigned int i = 1;
        for(const auto& app : PC.get_unique_support_list()){
            std::cout << i << ". " << app << std::endl;
            i++;
        }
    }


    else if(std::string(argv[1]) == "verify"){ // 'verify' param
        // TODO
    }


    else if(std::string(argv[1]) == "revert"){ // 'revert' param
        handleRevertOption(argv, pt, exePath, logfile);
    }


    else if(std::string(argv[1]) == "settings"){ // 'settings' param 
        
        if(argv[2] == NULL){ //* Default. No subparam provided.
            std::cout << ANSI_COLOR_YELLOW << "Settings: " << ANSI_COLOR_RESET << std::endl;

            std::cout << "Config save limit:            " << ANSI_COLOR_50 << pt.get<int>("savelimit") << ANSI_COLOR_RESET << std::endl;
            std::cout << "See 'cfgs settings.savelimit <limit>" << std::endl << std::endl;
            
            std::cout << "Backup recycle bin limit:     " << ANSI_COLOR_50 << pt.get<int>("recyclelimit") << ANSI_COLOR_RESET << std::endl;
            std::cout << "See 'cfgs settings.recyclelimit <limit>" << std::endl << std::endl;

            std::cout << "Scheduled Task On/Off:        " << ANSI_COLOR_50 << pt.get<bool>("task") << ANSI_COLOR_RESET << std::endl;
            std::cout << "See 'cfgs settings.task <True/False>" << std::endl << std::endl;

            std::cout << "Scheduled Task Frequency:     " << ANSI_COLOR_50 << pt.get<std::string>("taskfrequency") << ANSI_COLOR_RESET << std::endl;
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
            
            const std::string codeComm = "cmd /c code \"" + exePath + "\\settings.json\"";

            if(system(codeComm.c_str()) != 0){ // Open config with vscode
                const std::string notepadComm = "cmd /c notepad \"" + exePath + "\\settings.json\"";
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
                    std::cout << ANSI_COLOR_GREEN << "Task created!" << ANSI_COLOR_RESET << std::endl;
                }
            }

            else{ // Current property is already set to 'true'.
                std::cout << ANSI_COLOR_161 << "settings.task already set to true." << ANSI_COLOR_RESET << std::endl;
                std::cout <<  ANSI_COLOR_222 << "Checking task..." << ANSI_COLOR_RESET << std::endl;

                if(task::exists("task")){ // Task exists.
                    std::cout << "Task exists, no changes were made." << std::endl;
                }
                else{ // Task doesnt exist. --> launch task
                    std::cout << ANSI_COLOR_222 << "Task doesn't exist. Creating task..." << ANSI_COLOR_RESET << std::endl;
                    if(task::createtask(taskName, batPath, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);}
                    std::cout << ANSI_COLOR_GREEN << "Task created!" << ANSI_COLOR_RESET << std::endl;
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
    

    else{
        std::cerr << "Fatal: '" << argv[1] << "' is not a ConfigSync command.\n";
        return 1;
    }

    
    return 0;
}