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


int main(int argc, char* argv[]){   
    std::signal(SIGINT, exitSignalHandler);
    enableColors();
    
    pLocatFull = boost::dll::program_location().string();
    pLocat = boost::dll::program_location().parent_path().string();
    root = boost::dll::program_location().root_name().string();
    const std::string settingsPath = pLocat + "\\settings.json"; 
    CS::Logs logs;
    logs.init();

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
        if(task::createtask(taskName, pLocatFull, pt.get<std::string>("taskfrequency")) != 1){std::cerr << "Errror: failed to create task." << std::endl; exit(EXIT_FAILURE);} // Create task
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
    



    if(argv[1] == NULL){ // No params, display Copyright Notice
        std::cout << "ConfigSync (JW-CoreUtils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber. All rights reserved." << std::endl;
        std::cout << "Software Configuration Synchronizer." << std::endl;
        std::cout << "See 'cfgs --help' for usage." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    /* Parse operands */
    if(CS::Args::argcmp(argv, argc, "--verbose") == 1 || CS::Args::argcmp(argv, argc, "-v") == 1){
        verbose = 1;
    }

    /* Parse options: */
    if(std::string(argv[1]) == "version" || std::string(argv[1]) == "--version"){ // Version param
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

    else{
        std::cerr << "Fatal: '" << argv[1] << "' is not a ConfigSync command.\n";
        return 1;
    }

    
    return 0;
}