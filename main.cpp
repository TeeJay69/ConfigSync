#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>   
#include <cstdlib>
#include <unordered_map>
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
#include "ConfigSync.hpp"


#ifdef DEBUG
#define DEBUG_MODE 1
#else 
#define DEBUG_MODE 0
#endif

#define SETTINGS_ID 1
#define VERSION "v2.0.0"

volatile sig_atomic_t interrupt = 0;
int verbose = 0;
const std::string uName = std::getenv("username");
const int log_limit_extern = 60;
std::string pLocat;
std::string pLocatFull;
std::string root;
const std::string taskName = "ConfigSyncTask";
std::string archiveDir;
std::string savesFile;


std::string CS::Logs::logDir;
std::string CS::Logs::logPath;
std::ofstream CS::Logs::logf;

inline void CS::Logs::init(){
    logDir = pLocat + "\\logs.log";
    logPath = logDir + "\\" + timestamp();
    logf = create_log(logPath);
}

class Defaultsetting{
    public:
        const int savelimit = 60;
        const int recyclelimit = 60;
        const bool task = false;
        const std::string taskfrequency = "daily,1";

        void writeDefault(boost::property_tree::ptree& pt){
            pt.put("settingsID", SETTINGS_ID); // Settings identifier for updates
            pt.put("savelimit", savelimit);
            pt.put("recyclelimit", recyclelimit);
            pt.put("task", task);
            pt.put("taskfrequency", taskfrequency);
        }
};

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

inline void handleSyncOption(char* argv[], int argc, boost::property_tree::ptree& pt){
    CS::Programs::Mgm mgm;
    if(argv[2] == NULL){
        std::cerr << "Fatal: Missing argument or value." << std::endl;
    }
    else if(mgm.checkName(std::string(argv[2])) == 1){ // single program
        std::string msg;
        const char* cmp[] = {"--message", "-m"};
        size_t cmpSize = sizeof(cmp) / sizeof(cmp[0]);
        int msgFlag= 0;
        if(CS::Args::argfcmp(msg, argv, argc, cmp, cmpSize) == 1){
            msgFlag = 1;
        }

        const std::string canName = mgm.get_canonical(std::string(argv[2]));
    
        CS::Saves S(savesFile);
        S.load();
        uint64_t tst = CS::Utility::timestamp();
        const std::string dayDir = archiveDir + "\\" + canName + "\\" + CS::Utility::ymd_date();
        const std::string tstDir = dayDir + "\\" + std::to_string(tst);

        if(!std::filesystem::exists(tstDir)){
            std::filesystem::create_directories(tstDir);
        }
        else{
            CS::Filesystem::recurse_remove(tstDir);
            std::filesystem::create_directories(tstDir);
        }
        
        std::vector<std::pair<std::string,std::string>> pathvec;
        std::optional<std::reference_wrapper<std::vector<std::pair<std::string,std::string>>>> pvecRef = std::ref(pathvec);
        unsigned id = 1;
        for(const auto& el : mgm.programs()[canName].paths){
            const std::string dest = tstDir + "\\" + std::to_string(id);
            if(!std::filesystem::exists(el)){
                CS::Logs log;
                log.msg("Warning: Program path not found. " + el);
                continue;
            }
            CS::Filesystem::recurse_copy(el, dest, std::nullopt, pvecRef);
            id++;
        }

        if(msgFlag == 1){
            S.add(canName, uName, root, msg, pathvec, tst);
        }
        else{
            S.add(canName, uName, root, "", pathvec, tst);
        }

        if(S.saves()[canName].size() > pt.get<int>("savelimit")){
            uint64_t oldestTst = S.get_oldest_tst(canName);
            CS::Filesystem::recurse_remove(dayDir + "\\" + std::to_string(oldestTst));
            S.erase_save(canName, oldestTst);
        }
        
        S.save();
    }
    else if(std::string(argv[2]) == "--all" || std::string(argv[2]) == "-a"){
        std::string msg;
        const char* cmp[] = {"--message", "-m"};
        size_t cmpSize = sizeof(cmp) / sizeof(cmp[0]);
        int msgFlag= 0;
        if(CS::Args::argfcmp(msg, argv, argc, cmp, cmpSize == 1)){
            msgFlag = 1;
        }

        CS::Saves S(savesFile);
        S.load();

        for(const auto& prog : mgm.get_supported()){
            uint64_t tst = CS::Utility::timestamp();
            const std::string dayDir = archiveDir + "\\" + prog + "\\" + CS::Utility::ymd_date();
            const std::string tstDir = dayDir + "\\" + std::to_string(tst);
            if(!std::filesystem::exists(tstDir)){
            std::filesystem::create_directories(tstDir);
            }
            else{
                CS::Filesystem::recurse_remove(tstDir);
                std::filesystem::create_directories(tstDir);
            }
            std::vector<std::pair<std::string,std::string>> pathvec;
            std::optional<std::reference_wrapper<std::vector<std::pair<std::string,std::string>>>> pvecRef = std::ref(pathvec);
            unsigned id = 1;
            for(const auto& el : mgm.programs()[prog].paths){
                const std::string dest = tstDir + "\\" + std::to_string(id);
                if(!std::filesystem::exists(el)){
                    CS::Logs log;
                    log.msg("Warning: Program path not found. " + el);
                    continue;
                }
                CS::Filesystem::recurse_copy(el, dest, std::nullopt, pvecRef);
                id++;

                if(msgFlag == 1){
                    S.add(prog, uName, root, msg, pathvec, tst);
                }
                else{
                    S.add(prog, uName, root, "", pathvec, tst);
                }

                if(S.saves()[prog].size() > pt.get<int>("savelimit")){
                    uint64_t oldestTst = S.get_oldest_tst(prog);
                    CS::Filesystem::recurse_remove(dayDir + "\\" + std::to_string(oldestTst));
                    S.erase_save(prog, oldestTst);
                }
            }
        }

        S.save();
    }
}

inline void handleCheckOption(char* argv[], int argc, boost::property_tree::ptree& pt){
    if(argv[2] == NULL){
        std::cerr << "Fatal: Missing argument or value." << std::endl;
    }
    else if(std::string(argv[2]) == "--task" || std::string(argv[2]) == "-t"){
        /* Ensure that task setting reflects reality. */
        if(pt.get<bool>("task") == true && !CS::Task::exists(taskName)){ // Task setting is true and task doesnt exist
            if(CS::Task::create(taskName, pLocatFull, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);} 
        }
        else if(pt.get<bool>("task") == false && CS::Task::exists(taskName)){ // Task setting is false and task exists
            if(CS::Task::remove(taskName) != 1){std::cerr << "Errror: failed to remove task." << std::endl; exit(EXIT_FAILURE);} // Remove existing task
            std::exit(EXIT_FAILURE);
        }
        else if(pt.get<bool>("task") == false && !CS::Task::exists(taskName)){
            // Do nothing.
        }
        else if(pt.get<bool>("task") == true && CS::Task::exists(taskName)){
            // Do nothing.
        }
        else{
            std::cerr << "File: [" << __FILE__ << "] Line: [" << __LINE__ << "] This should never happen!\n";
        }
    }
    else{
        std::cerr << "Fatal: '" << argv[2] << "' is not a configsync command." << std::endl;
    }
}


int main(int argc, char* argv[]){   
    std::signal(SIGINT, exitSignalHandler);
    enableColors();
    
    pLocatFull = boost::dll::program_location().string();
    pLocat = boost::dll::program_location().parent_path().string();
    root = boost::dll::program_location().root_name().string();
    archiveDir = pLocat + "\\ConfigArchive";
    savesFile = pLocat + "\\objects\\Saves.bin";
    const std::string settingsPath = pLocat + "\\settings.json"; 
    CS::Logs::init();
    boost::property_tree::ptree pt;

    try{ // Try parsing the settings file.
        boost::property_tree::read_json(settingsPath, pt); // read config
    }

    catch(const boost::property_tree::json_parser_error& readError){ // Create default settings file.
        
        if(!std::filesystem::exists(settingsPath)){
            std::cout << "Initializing settings file..." << std::endl;
        }
        else{
            std::cerr << "Warning: error while parsing settings file (" << readError.what() << "). Restoring default settings..." << std::endl;
        }
        
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
        Defaultsetting DS;
        pt.put("settingsID", SETTINGS_ID);
        pt.put("recyclelimit", DS.recyclelimit);
        pt.put("savelimit", DS.savelimit);
        pt.put("task", DS.task);
        pt.put("taskfrequency", DS.taskfrequency);

    }

    else if(settingsID != SETTINGS_ID){ // Differing settingsID
        //! merge config.
        // Create the new config file after merging
    }
    else{
        // Do nothing, pt contains correct config
    }

    if(argv[1] == NULL){ // No params, display Copyright Notice
        std::cout << "ConfigSync (JW-CoreUtils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber. All rights reserved." << std::endl;
        std::cout << "Software Configuration Synchronizer." << std::endl;
        std::cout << "See 'cfgs --help' for usage." << std::endl;
        std::exit(EXIT_SUCCESS);
    }
    else if(CS::Args::argcmp(argv, argc, "--verbose") == 1 || CS::Args::argcmp(argv, argc, "-v") == 1){
        verbose = 1;
    }
    else if(std::string(argv[1]) == "version" || std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v"){ // Version param
        std::cout << "ConfigSync (JW-Coreutils) " << ANSI_COLOR_36 << VERSION << ANSI_COLOR_RESET << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber. All rights reserved." << std::endl;
        std::exit(EXIT_SUCCESS);
    }
    else if(std::string(argv[1]) == "help" || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"){ // Help message param
        std::cout << "ConfigSync (JW-CoreUtils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::cout << "usage: cfgs [OPTIONS]... [PROGRAM]" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:\n";
        std::cout << "sync [PROGRAM]                Create a snapshot of a program's configuration.\n";
        std::cout << "sync --all                    Create a snapshot for all supported programs.\n";
        std::cout << "revert [PROGRAM] [DATE]       Revert configuration of the specified program, date.\n";
        std::cout << "revert --all                  Revert configuration of all supported programs.\n";
        std::cout << "undo [PROGRAM] [DATE]         Undo a restore of the specified program, date.\n";
        std::cout << "undo --all                    Undo restore of all programs.\n";
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
        std::cout << "[...] --verbose, -v           Enable verbose mode.\n";
        if(verbose){std::cout << "Verbose mode enabled!!!"<<std::endl;}
        std::exit(EXIT_SUCCESS);
    }

    else if(std::string(argv[1]) == "sync"){
        handleSyncOption(argv, argc, pt);
    }

    else if(std::string(argv[1]) == "check"){
        handleCheckOption(argv, argc, pt);
    }
    else{
        std::cerr << "Fatal: '" << argv[1] << "' is not a ConfigSync command.\n";
        return 1;
    }

    
    return 0;
}