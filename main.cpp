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
#include <fstream>

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
std::string backupDir;

std::string CS::Logs::logDir;
std::string CS::Logs::logPath;
std::ofstream CS::Logs::logf;

inline void CS::Logs::init(){
    logDir = pLocat + "\\logs";
    logPath = logDir + "\\" + timestamp() + ".log";
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

inline void checkDynamicPath(CS::Saves& S, const std::string& prog, const uint64_t date){
    if(S.cmp_root(prog, date, root) != 1){
        S.replace_root(prog, date, root);
    }
    if(S.cmp_uname(prog, date, uName) != 1){
        S.replace_uname(prog, date, uName);
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
        std::cout << ANSI_COLOR_161 << "Synchronizing " << canName << ":" << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_222 << "Fetching files..." << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_222 << "Starting synchronization..." << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_222 << "Computing hashes..." << ANSI_COLOR_RESET << std::endl;
        CS::Saves S(savesFile);
        S.load();
        unsigned loadFlag = 1;
        if(S.load() != 1){
            loadFlag = 0;
        }
        int equal = 1;
        if(loadFlag != 0){
            if(S.exists(canName)){
                for(const auto& pair : S.get_lastsave(canName).pathVec){
                    if(CS::Utility::get_sha256hash(pair.first) != CS::Utility::get_sha256hash(pair.second)){
                        equal = 0;
                        break;
                    }
                }
            }
            else{
                equal = 0;
            }
        }
        else{
            equal = 0;
        }

        if(equal == 1){
            std::cout << ANSI_COLOR_66 << canName + " config is up to date." << ANSI_COLOR_RESET << std::endl;
            CS::Logs::msg("Hashes identical: " + canName);
            return;
        }
        std::cout << ANSI_COLOR_138 << canName << " config is out of sync! Synchronizing..." << ANSI_COLOR_RESET << std::endl;
        CS::Logs::msg("Hashes differ: " + canName);
    
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
        for(const auto& el : mgm.programs().at(canName).paths){
            const std::string dest = tstDir + "\\" + std::to_string(id);
            if(!std::filesystem::exists(el)){
                CS::Logs::msg("Warning: Program path not found. [" + el + "]");
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

        if(S.saves().at(canName).size() > pt.get<int>("savelimit")){
            uint64_t oldestTst = S.get_oldest_tst(canName);
            CS::Filesystem::recurse_remove(dayDir + "\\" + std::to_string(oldestTst));
            S.erase_save(canName, oldestTst);
        }

        std::cout << ANSI_COLOR_GREEN << "Synchronization of " << canName << " finished." << ANSI_COLOR_RESET << std::endl;
        S.save();
    }
    else if(std::string(argv[2]) == "--all" || std::string(argv[2]) == "-a"){
        std::cout << ANSI_COLOR_161 << "Synchronizing all programs:" << ANSI_COLOR_RESET << std::endl;
        std::string msg;
        const char* cmp[] = {"--message", "-m"};
        size_t cmpSize = sizeof(cmp) / sizeof(cmp[0]);
        int msgFlag= 0;
        if(CS::Args::argfcmp(msg, argv, argc, cmp, cmpSize == 1)){
            msgFlag = 1;
        }

        std::cout << ANSI_COLOR_222 << "Fetching programs..." << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_222 << "Starting synchronization..." << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_222 << "Computing hashes..." << ANSI_COLOR_RESET << std::endl;
        CS::Saves S(savesFile);
        unsigned loadFlag = 1;
        if(S.load() != 1){
            loadFlag = 0;
        }
        std::vector<std::string> synced;
        for(const auto& prog : mgm.get_supported()){
            unsigned equal = 1;
            if(loadFlag != 0){
                if(S.exists(prog)){
                    for(const auto& pair : S.get_lastsave(prog).pathVec){
                        if(CS::Utility::get_sha256hash(pair.first) != CS::Utility::get_sha256hash(pair.second)){
                            equal = 0;
                            break;
                        }
                    }
                }
                else{
                    equal = 0;
                }
            }
            else{
                equal = 0;
            }

            if(equal == 1){
                std::cout << ANSI_COLOR_66 << prog + " config is up to date." << ANSI_COLOR_RESET << std::endl;
                CS::Logs::msg("Hashes identical: " + prog);
                continue;
            }
            std::cout << ANSI_COLOR_138 << prog << " config is out of sync! Synchronizing..." << ANSI_COLOR_RESET << std::endl;
            CS::Logs::msg("Hashes differ: " + prog);
            
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
            }
            // // for(const auto& pair : pathvec){
            //     std::cout << "pathvec: " << pair.first << std::endl;
            if(msgFlag == 1){
                S.add(prog, uName, root, msg, pathvec, tst);
            }
            else{
                S.add(prog, uName, root, "", pathvec, tst);
            }

            if(S.saves().at(prog).size() > pt.get<int>("savelimit")){
                uint64_t oldestTst = S.get_oldest_tst(prog);
                CS::Filesystem::recurse_remove(dayDir + "\\" + std::to_string(oldestTst));
                S.erase_save(prog, oldestTst);
            }
            synced.push_back(prog);
        }
        std::cout << ANSI_COLOR_222 << "Preparing summary..." << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_161 << "Synced programs:" << ANSI_COLOR_RESET << std::endl;
        unsigned i = 1;
        for(const auto& elem : synced){
            std::cout << ANSI_COLOR_GREEN << i << "." << elem << ANSI_COLOR_RESET << std::endl;
            i++;
        }
        std::cout << ANSI_COLOR_GREEN << "Synchronization finished!" << ANSI_COLOR_RESET << std::endl;
        S.save();
    }
    else{
        std::cerr << "Fatal: '" << argv[2] << "' is not a ConfigSync command. See 'configsync --help'." << std::endl;
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
        std::cerr << "Fatal: '" << argv[2] << "' is not a ConfigSync command. See 'configsync --help'." << std::endl;
    }
}

inline uint64_t getNearestDate(CS::Saves& S, const std::string& prog, const uint64_t date){
    if(!S.exists(prog)){
        return 0;
    }
    
    if(S.find_save(prog, date) == 1){
        return date;
    }

    auto it = S.saves().at(prog).upper_bound(date);

    if(it == S.saves().at(prog).end()){
        return 0;
    }

    return it--->first;
}

inline uint64_t getNearestResDate(CS::Saves& S, const std::string& prog, const uint64_t date){
    if(!S.exists(prog)){
        return 0;
    }

    if(S.find_save(prog, date) == 1){
        if(S.saves().at(prog).at(date).message == "Pre-Restore-Backup"){
            return date;
        }
    }

    auto it = S.saves().at(prog).upper_bound(date);

    while(true){
        if(it == S.saves().at(prog).end()){
            return 0;
        }
        if(it--->second.message != "Pre-Restore-Backup"){
            it--;
        }
        else{
            break;
        }
    }
    return it->first;
}

inline uint64_t getNearestSaveDate(CS::Saves& S, const std::string& prog, const uint64_t date){
    if(!S.exists(prog)){
        return 0;
    }

    if(S.find_save(prog, date) == 1){
        if(S.saves().at(prog).at(date).message != "Pre-Restore-Backup"){
            return date;
        }
    }

    auto it = S.saves().at(prog).upper_bound(date);

    while(true){
        if(it == S.saves().at(prog).end()){
            return 0;
        }
        if(it--->second.message == "Pre-Restore-Backup"){
            it--;
        }
        else{
            break;
        }
    }
    return it->first;
}

inline int createSave(CS::Programs::Mgm& mgm, CS::Saves& S, const std::string& prog, const std::string& destDayDir, const uint64_t tst, const std::string& message){
    const std::string tstDir = destDayDir + "\\" + std::to_string(tst);
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
    }

    S.add(prog, uName, root, message, pathvec, tst);

    return 1;
}

inline int preRestoreBackup(CS::Programs::Mgm& mgm, CS::Saves& S, const std::string& prog){
    uint64_t tst = CS::Utility::timestamp();
    const std::string backupDayDir = backupDir + "\\" + prog + "\\" + CS::Utility::ymd_date();
    if(createSave(mgm, S, prog, backupDayDir, tst, "Pre-Restore-Backup") != 1){
        return 0;
    }
    return 1;
}

inline int restoreSave(CS::Saves& S, const std::string& prog, const uint64_t date){
    checkDynamicPath(S, prog, date);
    for(const auto& pair : S.saves().at(prog).at(date).pathVec){
        if(std::filesystem::exists(pair.first)){
            if(std::filesystem::is_directory(pair.first)){
                CS::Filesystem::recurse_remove(pair.first);
            }
            else{
                std::filesystem::permissions(pair.first, std::filesystem::perms::all);
                std::filesystem::remove(pair.first);
            }
        }
        std::filesystem::permissions(pair.second, std::filesystem::perms::all);
        if(std::filesystem::copy_file(pair.second, pair.first, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::update_existing) != 1){
            return 0;
        }
    }

    return 1;
}

inline int removeSave(CS::Saves& S, const std::string& prog, const uint64_t date){
    checkDynamicPath(S, prog, date);
    for(const auto& [first, second] : S.saves().at(prog).at(date).pathVec){
        if(std::filesystem::exists(second)){
            if(std::filesystem::is_directory(second)){
                CS::Filesystem::recurse_remove(second);
            }
            else{
                std::filesystem::permissions(second, std::filesystem::perms::all);
                std::filesystem::remove(second);
            }
        }
    }
    if(!S.erase_save(prog, date)){
        return 0;
    }
    return 1;
}

inline void handleRestoreOption(char* argv[], int argc){
    CS::Programs::Mgm mgm;
    if(argv[2] == NULL){
        std::cerr << "Fatal: Missing argument or value." << std::endl;
    }
    else if(mgm.checkName(std::string(argv[2])) == 1){
        const std::string canName = mgm.get_canonical(std::string(argv[2]));
        CS::Saves S(savesFile);
        if(S.load() != 1){
            std::cerr << "Fatal: No previous saves to restore." << std::endl;
            return;
        }

        const char* cmp[] = {"--date", "-d"};
        const size_t cmpSize = sizeof(cmp) / sizeof(cmp[0]);
        std::string dateVal;
        if(CS::Args::argfcmp(dateVal, argv, argc, cmp, cmpSize) == 1){
            if(CS::Args::argcmp(argv, argc, "--force") == 1 || CS::Args::argcmp(argv, argc, "-f") == 1){
                std::cout << ANSI_COLOR_161 << "Restoring config of " << canName << " from " << dateVal << " - Forced:" << std::endl;
                for(const auto& proc : mgm.programs().at(canName).procNames){
                    if(CS::Process::killProcess(proc.c_str()) == 1){
                        std::cout << ANSI_COLOR_222 << "Killed " << proc << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }
            else{
                std::cout << ANSI_COLOR_161 << "Restoring config of " << canName << " from " << dateVal << ":" << std::endl;
            }
            const uint64_t dateTst = CS::Utility::str_to_timestamp<uint64_t>(dateVal);
            const uint64_t date = getNearestDate(S, canName, dateTst);
            if(date == 0 || S.find_save(canName, date) == 0){
                std::cout << "Fatal: No save of " << canName << " found." << std::endl;
                return;
            }
            
            if(preRestoreBackup(mgm, S, canName) != 1){
                std::cerr << ANSI_COLOR_RED << "Error: Failed to create Pre-Restore-Backup, terminating..." << ANSI_COLOR_RESET << std::endl; 
                std::exit(EXIT_FAILURE);
            }
            else{
                std::cout << ANSI_COLOR_222 << "Created Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
            }

            if(restoreSave(S, canName, date) != 1){
                std::cerr << ANSI_COLOR_RED << "Failed to restore save." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_222 << "Restoring from Pre-Restore-Backup..." << ANSI_COLOR_RESET << std::endl;
                if(restoreSave(S, canName, S.get_last_tst(canName)) != 1){
                    std::cerr << ANSI_COLOR_RED << "Error: Failed to restore from Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
                    std::cerr << "Please ensure the target program is functioning normally." << std::endl;
                    std::exit(EXIT_FAILURE);
                }
            }
            std::cout << ANSI_COLOR_GREEN << "Restore successfull." << ANSI_COLOR_RESET << std::endl;
            S.save();
        }
        else{
            if(S.exists(canName) != 1){
                std::cerr << "Fatal: No save of " << canName << " found." << std::endl;
                return;
            }
            const uint64_t date = S.get_last_tst(canName);
            if(CS::Args::argcmp(argv, argc, "--force") == 1 || CS::Args::argcmp(argv, argc, "-f") == 1){
                for(const auto& proc : mgm.programs().at(canName).procNames){
                    if(CS::Process::killProcess(proc.c_str()) == 1){
                        std::cout << ANSI_COLOR_222 << "Killed " <<  proc << ANSI_COLOR_RESET << std::endl;
                    }
                }
                std::cout << ANSI_COLOR_161 << "Restoring config of " << canName << " - Forced:" << std::endl;
            }
            else{
                std::cout << ANSI_COLOR_161 << "Restoring config of " << canName << ":" << std::endl;
            }

            if(preRestoreBackup(mgm, S, canName) != 1){
                std::cerr << ANSI_COLOR_RED << "Error: Failed to create Pre-Restore-Backup, terminating..." << ANSI_COLOR_RESET << std::endl; 
                std::exit(EXIT_FAILURE);
            }
            else{
                std::cout << ANSI_COLOR_222 << "Created Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
            }
            if(restoreSave(S, canName, date) != 1){
                std::cerr << ANSI_COLOR_RED << "Failed to restore save." << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_222 << "Restoring from Pre-Restore-Backup..." << ANSI_COLOR_RESET << std::endl;
                if(restoreSave(S, canName, S.get_last_tst(canName)) != 1){
                    std::cerr << ANSI_COLOR_RED << "Error: Failed to restore from Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
                    std::cerr << "Please ensure the target program is functioning normally." << std::endl;
                    std::exit(EXIT_FAILURE);
                }
            }
            std::cout << ANSI_COLOR_GREEN << "Restore successfull." << ANSI_COLOR_RESET << std::endl;
            S.save();
        }
    }
    else if(CS::Args::argcmp(argv, argc, "--all") == 1 || CS::Args::argcmp(argv, argc, "-a") == 1){
        const char* cmp[] = {"--date", "-d"};
        const size_t cmpSize = sizeof(cmp) / sizeof(cmp[0]);
        std::string dateVal;
        if(CS::Args::argfcmp(dateVal, argv, argc, cmp, cmpSize) == 1){
            const uint64_t dateTst = CS::Utility::str_to_timestamp<uint64_t>(dateVal);
            CS::Saves S(savesFile);
            if(S.load() != 1){
                std::cerr << "Fatal: No previous saves to restore." << std::endl;
                return;
            }

            int forceFlag = 0;
            if(CS::Args::argcmp(argv, argc, "--force") == 1 || CS::Args::argcmp(argv, argc, "-f") == 1){
                std::cout << ANSI_COLOR_161 << "Restoring config of all programs from " << dateVal << " - Forced:" << std::endl;
                forceFlag = 1;
            }
            else{
                std::cout << ANSI_COLOR_161 << "Restoring config of all programs from " << dateVal << ":" << ANSI_COLOR_RESET << std::endl;
            }

            std::vector<std::string> success;
            std::vector<std::string> failed;
            for(const auto& prog : mgm.get_supported()){
                const uint64_t date = getNearestDate(S, prog, dateTst);
                if(date == 0 || S.find_save(prog, date) == 0){
                    std::cout << ANSI_COLOR_YELLOW << "No save of " << prog << " found, skipping..." << ANSI_COLOR_RESET << std::endl;
                    failed.emplace_back(prog);
                    continue;
                }
                
                if(forceFlag == 1){
                    for(const auto& proc : mgm.programs().at(prog).procNames){
                        if(CS::Process::killProcess(proc.c_str()) == 1){
                            std::cout << ANSI_COLOR_222 << "Killed " << proc << ANSI_COLOR_RESET << std::endl;
                        }
                    }
                }

                if(preRestoreBackup(mgm, S, prog) != 1){
                    std::cerr << ANSI_COLOR_RED << "Error: Failed to create Pre-Restore-Backup, terminating..." << ANSI_COLOR_RESET << std::endl; 
                    std::exit(EXIT_FAILURE);
                }
                else{
                    std::cout << ANSI_COLOR_222 << "Created Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
                }
                
                if(restoreSave(S, prog, date) != 1){
                    failed.push_back(prog);
                    std::cerr << ANSI_COLOR_RED << "Failed to restore save." << ANSI_COLOR_RESET << std::endl;
                    std::cout << ANSI_COLOR_222 << "Restoring from Pre-Restore-Backup..." << ANSI_COLOR_RESET << std::endl;
                    if(restoreSave(S, prog, S.get_last_tst(prog)) != 1){
                        std::cerr << ANSI_COLOR_RED << "Error: Failed to restore from Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
                        std::cerr << "Please ensure the target program is functioning normally." << std::endl;
                        S.save();
                        std::exit(EXIT_FAILURE);
                    }
                }
                else{
                    success.push_back(prog);
                }
            }
            std::cout << ANSI_COLOR_222 << "Restore complete." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Preparing summary..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_161 << "Restored programs:" << ANSI_COLOR_RESET << std::endl;
            unsigned i = 0;
            for(const auto& el : success){
                std::cout << ANSI_COLOR_GREEN << i << ". " << el << ANSI_COLOR_RESET << std::endl;
                i++;
            }

            if(!failed.empty()){
                std::cout << ANSI_COLOR_RED << "Failed to restore:" << ANSI_COLOR_RESET << std::endl;
                unsigned ii = 0;
                for(const auto& el : failed){
                    std::cout << ANSI_COLOR_RED << ii << ". " << el << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
            }

            S.save();
        }
        else{
            CS::Saves S(savesFile);
            if(S.load() != 1){
                std::cerr << "Fatal: No previous saves to restore." << std::endl;
                return;
            }

            int forceFlag = 0;
            if(CS::Args::argcmp(argv, argc, "--force") == 1 || CS::Args::argcmp(argv, argc, "-f") == 1){
                std::cout << ANSI_COLOR_161 << "Restoring config of all programs from latest save - Forced:" << std::endl;
                forceFlag = 1;
            }
            else{
                std::cout << ANSI_COLOR_161 << "Restoring config of all programs from latest save:" << ANSI_COLOR_RESET << std::endl;
            }

            std::vector<std::string> success;
            std::vector<std::string> failed;
            for(const auto& prog : mgm.get_supported()){
                const uint64_t date = S.get_last_tst(prog);
                if(date == 0 || S.find_save(prog, date) == 0){
                    std::cout << ANSI_COLOR_YELLOW << "No save of " << prog << " found, skipping..." << ANSI_COLOR_RESET << std::endl;
                    failed.emplace_back(prog);
                    continue;
                }
                
                if(forceFlag == 1){
                    for(const auto& proc : mgm.programs().at(prog).procNames){
                        if(CS::Process::killProcess(proc.c_str()) == 1){
                            std::cout << ANSI_COLOR_222 << "Killed " << proc << ANSI_COLOR_RESET << std::endl;
                        }
                    }
                }

                if(preRestoreBackup(mgm, S, prog) != 1){
                    std::cerr << ANSI_COLOR_RED << "Error: Failed to create Pre-Restore-Backup, terminating..." << ANSI_COLOR_RESET << std::endl; 
                    std::exit(EXIT_FAILURE);
                }
                else{
                    std::cout << ANSI_COLOR_222 << "Created Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
                }
                
                if(restoreSave(S, prog, date) != 1){
                    failed.push_back(prog);
                    std::cerr << ANSI_COLOR_RED << "Failed to restore save." << ANSI_COLOR_RESET << std::endl;
                    std::cout << ANSI_COLOR_222 << "Restoring from Pre-Restore-Backup..." << ANSI_COLOR_RESET << std::endl;
                    if(restoreSave(S, prog, S.get_last_tst(prog)) != 1){
                        std::cerr << ANSI_COLOR_RED << "Error: Failed to restore from Pre-Restore-Backup" << ANSI_COLOR_RESET << std::endl;
                        std::cerr << "Please ensure the target program is functioning normally." << std::endl;
                        S.save();
                        std::exit(EXIT_FAILURE);
                    }
                }
                else{
                    success.push_back(prog);
                }
            }
            std::cout << ANSI_COLOR_222 << "Restore complete." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_222 << "Preparing summary..." << ANSI_COLOR_RESET << std::endl;
            std::cout << ANSI_COLOR_161 << "Restored programs:" << ANSI_COLOR_RESET << std::endl;
            unsigned i = 0;
            for(const auto& el : success){
                std::cout << ANSI_COLOR_GREEN << i << ". " << el << ANSI_COLOR_RESET << std::endl;
                i++;
            }

            if(!failed.empty()){
                std::cout << ANSI_COLOR_RED << "Failed to restore:" << ANSI_COLOR_RESET << std::endl;
                unsigned ii = 0;
                for(const auto& el : failed){
                    std::cout << ANSI_COLOR_RED << ii << ". " << el << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
            }

            S.save();
        }
    }
    else{
        std::cerr << "Fatal: Missing argument or value. See 'configsync --help'" << std::endl;
    }
}


inline void handleShowOption(char* argv[], int argc){
    CS::Programs::Mgm mgm;
    if(argv[2] == NULL){
        std::cerr << "Fatal: Missing argument or value." << std::endl;
    }
    else if(mgm.checkName(std::string(argv[2])) == 1){
        const std::string canName = mgm.get_canonical(std::string(argv[2]));
        CS::Saves S(savesFile);
        if(S.load() == 0){
            std::cerr << "Fatal: No previous saves found. See 'configsync --help'." << std::endl;
            return;
        }

        std::cout << ANSI_COLOR_222 << canName << ":" << ANSI_COLOR_RESET << std::endl;
        std::cout << ANSI_COLOR_166 << "Saves:" << ANSI_COLOR_RESET << std::endl;
        std::vector<uint64_t> resVec; 
        unsigned sCount = 0;
        unsigned pCount = 0;
        unsigned i = 1;
        for(const auto& save : S.saves().at(canName)){
            if(save.second.message != "Pre-Restore-Backup"){
                if(!save.second.message.empty()){
                    std::cout << ANSI_COLOR_146 << i << ". " << CS::Utility::timestamp_to_str(save.first) << ANSI_COLOR_RESET << " - " << save.second.message << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_146 << i << ". " << CS::Utility::timestamp_to_str(save.first) << ANSI_COLOR_RESET << std::endl;
                }
                sCount++;
            }
            else{
                resVec.emplace_back(save.first);
                pCount++;
            }
            i++;
        }
        std::cout << std::endl;
        if(!resVec.empty()){
            std::cout << ANSI_COLOR_166 << "Pre-Restore-Backups:" << ANSI_COLOR_RESET << std::endl;
            unsigned ii = 1;
            for(const auto& tst : resVec){
                std::cout << ANSI_COLOR_151 << ii << ". " << CS::Utility::timestamp_to_str(tst) << ANSI_COLOR_RESET << std::endl;
                ii++;
            }
            if(sCount == 1){
                std::cout << "Found " << ANSI_COLOR_GREEN << sCount << ANSI_COLOR_RESET << " save and ";
            }
            else{
                std::cout << "Found " << ANSI_COLOR_GREEN <<  sCount << ANSI_COLOR_RESET << " saves and ";
            }
            if(pCount == 1){
                std::cout << ANSI_COLOR_GREEN << pCount << ANSI_COLOR_RESET << " Pre-Restore-Backup." << std::endl; 
            }
            else{
                std::cout << ANSI_COLOR_GREEN << pCount << ANSI_COLOR_RESET << " Pre-Restore-Backups." << std::endl;
            }
        }
        else{
            if(sCount == 1){
                std::cout << "Found " << ANSI_COLOR_GREEN << sCount << ANSI_COLOR_RESET << " save." << std::endl;
            }
            else{
                std::cout << "Found " << ANSI_COLOR_GREEN << sCount << ANSI_COLOR_RESET << " saves." << std::endl;
            }
        }
    }
    else if(CS::Args::argcmp(argv, argc, "--explorer") == 1 || CS::Args::argcmp(argv, argc, "-e") == 1){
        const std::string com = "cmd /c \"explorer \"" + archiveDir + "\"\"";
        const std::string pwshcom = "pwsh /c explorer \"" + archiveDir + "\"";
        if(std::system(com.c_str()) != 1){ // Explorer exit code is 1 for success
            std::cerr << ANSI_COLOR_RED << "Failed to call windows command processor." << ANSI_COLOR_RESET << std::endl;
            std::cout << "Trying to call powershell..." << std::endl;
            if(std::system(pwshcom.c_str()) != 1){
                std::cerr << ANSI_COLOR_RED << "Failed to call powershell, terminating..." << ANSI_COLOR_RESET << std::endl;
            }
                std::exit(EXIT_FAILURE);
        }
        return;
    }
    else{
        std::cerr << "Fatal: Missing argument or value. See 'configsync --help'" << std::endl;
    }
}

inline void handleStatusOption(char* argv[], int argc){
    CS::Programs::Mgm mgm;
    if(argv[2] == NULL){
        std::cout << ANSI_COLOR_166 << "Status:" << ANSI_COLOR_RESET << std::endl;
        CS::Saves S(savesFile);
        if(!S.load()){
            std::cout << ANSI_COLOR_RED << "Never synced:" << std::endl;
            for(const auto& prog : mgm.get_supported()){
                std::cout << prog << ANSI_COLOR_RESET << std::endl;
            }
        }
        std::vector<std::string> insync;
        std::vector<std::string> outsync;
        std::vector<std::string> nsync;
        for(const auto& prog : mgm.get_supported()){
            if(!S.exists(prog)){
                nsync.push_back(prog);
                continue;
            }
            uint64_t tst = S.get_last_tst(prog);
            if(tst == 0){
                std::cerr << "Warning: This should never happen. Failed to retrieve last timestamp for " << prog << std::endl;
                CS::Logs::msg("Warning: This should never happen. Failed to retrieve last timestamp for " + prog);
            }

            unsigned int equal = 1;
            for(const auto& pair : S.saves().at(prog).at(tst).pathVec){
                if(CS::Utility::get_sha256hash(pair.first) != CS::Utility::get_sha256hash(pair.second)){
                    outsync.push_back(prog);
                    equal = 0;
                    break;
                }
            }

            if(equal == 1){
                nsync.push_back(prog);
            }
        }

        if(!insync.empty()){
            std::cout << ANSI_COLOR_GREEN << "In sync:" << ANSI_COLOR_RESET << std::endl;
            for(const auto& el : insync){
                std::cout << ANSI_COLOR_GREEN << el << ANSI_COLOR_RESET << std::endl;
            }
        }
        if(!outsync.empty()){
            std::cout << ANSI_COLOR_YELLOW << "Out of sync:" << ANSI_COLOR_RESET << std::endl;
            for(const auto& el : outsync){
                std::cout << ANSI_COLOR_YELLOW << el << ANSI_COLOR_RESET << std::endl;
            }
        }
        if(!nsync.empty()){
            std::cout << ANSI_COLOR_RED << "Never synced:" << ANSI_COLOR_RESET << std::endl;
            for(const auto& el : nsync){
                std::cout << ANSI_COLOR_RED << el << ANSI_COLOR_RESET << std::endl;
            }
        }
    }
    else if(mgm.checkName(std::string(argv[2])) == 1){
        const std::string canName = mgm.get_canonical(std::string(argv[2]));
        std::cout << ANSI_COLOR_166 << "Status:" << ANSI_COLOR_RESET << std::endl;
        CS::Saves S(savesFile);
        if(!S.load()){
            std::cout << ANSI_COLOR_RED << "Never synced: " << canName << std::endl;
            return;
        }
        if(!S.exists(canName)){
            std::cerr << "Fatal: No saves of " << canName << " found." << std::endl;
        }

        uint64_t tst = S.get_last_tst(canName);
        if(tst == 0){
            std::cerr << "Warning: This should never happen. Failed to retrieve last timestamp for " << canName << std::endl;
            CS::Logs::msg("Warning: This should never happen. Failed to retrieve last timestamp for " + canName);
        }

        for(const auto& pair : S.saves().at(canName).at(tst).pathVec){
            if(CS::Utility::get_sha256hash(pair.first) != CS::Utility::get_sha256hash(pair.second)){
                std::cout << ANSI_COLOR_RED << "Out of Sync!" << ANSI_COLOR_RESET << std::endl;
                return;
            }
        }
        std::cout << ANSI_COLOR_GREEN << "In sync!" << ANSI_COLOR_RESET << std::endl;
    }
    else{
        std::cerr << "Fatal: '" << argv[2] << "' is not a supported operand." << std::endl;
    }
}

inline int getArgsName(std::string& buff, char* argv[], int argc){
    CS::Programs::Mgm mgm;
    for(int i = 0; i < argc; i++){
        if(mgm.checkName(std::string(argv[i])) == 1){
            buff = mgm.get_canonical(std::string(argv[i]));
            return 1;
        }
    }
    return 0;
}

inline void handleUndoOption(char* argv[], int argc){
    CS::Programs::Mgm mgm;
    if(argv[2] == NULL){
        std::cerr << "Fatal: Missing argument or value." << std::endl;
    }
    else if(CS::Args::argcmp(argv, argc, "--all") || CS::Args::argcmp(argv, argc, "-a")){
        if(CS::Args::argcmp(argv, argc, "--restore") || CS::Args::argcmp(argv, argc, "-r")){
            int forceFlag = 0;
            if(CS::Args::argcmp(argv, argc, "--force") || CS::Args::argcmp(argv, argc, "-f")){
                forceFlag = 1;
            }
            if(std::string dateVal; CS::Args::argfcmp(dateVal, argv, argc, "--date") || CS::Args::argfcmp(dateVal, argv, argc, "-d")){
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                std::vector<std::string> success;
                std::vector<std::string> fail;
                if(forceFlag){
                    std::cout << ANSI_COLOR_166 << "Undo restore action for all, with date - Forced:" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_166 << "Undo restore action for all, with date:" << ANSI_COLOR_RESET << std::endl;
                }
                for(const auto& prog : mgm.get_supported()){
                    const uint64_t tstNotNear = CS::Utility::str_to_timestamp<uint64_t>(dateVal);
                    const uint64_t tst = getNearestResDate(S, prog, tstNotNear);
                    if(tst == 0 || !S.find_save(prog, tst)){
                        std::cout << ANSI_COLOR_YELLOW << prog << " date not found, skipping..." << ANSI_COLOR_RESET << std::endl;
                        fail.push_back(prog);
                        continue;
                    }
                    else if(S.saves().at(prog).at(tst).message != "Pre-Restore-Backup"){ // ! Remove before release.
                        std::cout << prog << " nearest date is unrelated to a restore, skipping..." << std::endl;
                        fail.push_back(prog);
                        continue;
                    }
                    
                    if(forceFlag){
                        for(const auto& proc : mgm.programs().at(prog).procNames){
                            if(CS::Process::killProcess(proc.c_str())){
                                std::cout << ANSI_COLOR_222 << "Killed " << proc << "." << ANSI_COLOR_RESET << std::endl;
                            }
                        }
                    }

                    if(restoreSave(S, prog, tst) != 1){
                        std::cerr << ANSI_COLOR_RED << "Failed to undo restore " << prog << "." << ANSI_COLOR_RESET << std::endl;
                        fail.push_back(prog);
                        continue;
                    }

                    success.push_back(prog);
                }
                
                if(!fail.empty()){
                    std::cout << ANSI_COLOR_RED << "Failed:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : fail){
                        std::cout << ANSI_COLOR_RED << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                if(!success.empty()){
                    std::cout << ANSI_COLOR_GREEN << "Success:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : success){
                        std::cout << ANSI_COLOR_GREEN << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }
            else{
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                std::vector<std::string> success;
                std::vector<std::string> fail;
                if(forceFlag){
                    std::cout << ANSI_COLOR_166 << "Undo restore action for all - Forced:" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_166 << "Undo restore action for all:" << ANSI_COLOR_RESET << std::endl;
                }
                for(const auto& prog : mgm.get_supported()){
                    const uint64_t tst = getNearestResDate(S, prog, S.get_last_tst(prog));
                    if(tst == 0 || !S.find_save(prog, tst)){
                        std::cout << ANSI_COLOR_YELLOW << prog << " date not found, skipping..." << ANSI_COLOR_RESET << std::endl;
                        fail.push_back(prog);
                        continue;
                    }
                    else if(S.saves().at(prog).at(tst).message != "Pre-Restore-Backup"){ // ! Remove before release.
                        std::cout << prog << " nearest date is unrelated to a restore, skipping..." << std::endl;
                        fail.push_back(prog);
                        continue;
                    }

                    if(forceFlag){
                        for(const auto& proc : mgm.programs().at(prog).procNames){
                            if(CS::Process::killProcess(proc.c_str())){
                                std::cout << ANSI_COLOR_222 << "Killed " << proc << "." << ANSI_COLOR_RESET << std::endl;
                            }
                        }
                    }

                    if(restoreSave(S, prog, tst) != 1){
                        std::cerr << ANSI_COLOR_RED << "Failed to undo restore action of " << prog << "." << ANSI_COLOR_RESET << std::endl;
                        fail.push_back(prog);
                        continue;
                    }

                    success.push_back(prog);
                }
                
                if(!fail.empty()){
                    std::cout << ANSI_COLOR_RED << "Failed:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : fail){
                        std::cout << ANSI_COLOR_RED << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                if(!success.empty()){
                    std::cout << ANSI_COLOR_GREEN << "Success:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : success){
                        std::cout << ANSI_COLOR_GREEN << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
            }
        }
        else if(CS::Args::argcmp(argv, argc, "--save") || CS::Args::argcmp(argv, argc, "-s")){
            if(std::string dateVal; CS::Args::argfcmp(dateVal, argv, argc, "--date") || CS::Args::argfcmp(dateVal, argv, argc, "-d")){
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                std::cout << ANSI_COLOR_166 << "Undo save action for all, with date:" << ANSI_COLOR_RESET << std::endl;
                std::vector<std::string> success;
                std::vector<std::string> neverSync;
                std::vector<std::string> fail;
                for(const auto& prog : mgm.get_supported()){
                    const uint64_t dateNotNear = CS::Utility::str_to_timestamp<uint64_t>(dateVal);
                    const uint64_t tst = getNearestSaveDate(S, prog, dateNotNear);
                    if(tst == 0 || S.find_save(prog, tst) != 1){
                        std::cerr << ANSI_COLOR_YELLOW << prog << " date not found, skipping..." << ANSI_COLOR_RESET << std::endl;
                        if(S.saves().at(prog).empty()){
                            neverSync.push_back(prog);
                        }
                        else{
                            fail.push_back(prog);
                        }
                        continue;
                    }

                    if(!removeSave(S, prog, tst)){
                        std::cerr << ANSI_COLOR_RED << "Failed to remove save of " << prog << " from " << dateVal << ANSI_COLOR_RESET << std::endl;
                        fail.push_back(prog);
                    }
                    else{
                        success.push_back(prog);
                    }
                }

                if(!fail.empty()){
                    std::cout << ANSI_COLOR_RED << "Failed:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : fail){
                        std::cout << ANSI_COLOR_RED << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                if(!neverSync.empty()){
                    std::cout << ANSI_COLOR_YELLOW << "Never synced:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : neverSync){
                        std::cout << ANSI_COLOR_YELLOW << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                if(!success.empty()){
                    std::cout << ANSI_COLOR_GREEN << "Success:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : success){
                        std::cout << ANSI_COLOR_GREEN << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                S.save();
            }
            else{
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                std::cout << ANSI_COLOR_166 << "Undo save action for all:" << ANSI_COLOR_RESET << std::endl;
                std::vector<std::string> success;
                std::vector<std::string> fail;
                std::vector<std::string> neverSync;
                for(const auto& prog : mgm.get_supported()){
                    const uint64_t tst = getNearestSaveDate(S, prog, S.get_last_tst(prog));
                    if(tst == 0 || !S.find_save(prog, tst)){
                        std::cerr << ANSI_COLOR_YELLOW << prog << " no save found, skipping..." << ANSI_COLOR_RESET << std::endl;
                        neverSync.push_back(prog);
                        continue;
                    }

                    if(!removeSave(S, prog, tst)){
                        std::cerr << ANSI_COLOR_RED << "Failed to remove save of " << prog << ANSI_COLOR_RESET << std::endl;
                        fail.push_back(prog);
                    }
                    else{
                        success.push_back(prog);
                    }
                }

                if(!fail.empty()){
                    std::cout << ANSI_COLOR_RED << "Failed:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : fail){
                        std::cout << ANSI_COLOR_RED << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                if(!neverSync.empty()){
                    std::cout << ANSI_COLOR_YELLOW << "Never synced:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : neverSync){
                        std::cout << ANSI_COLOR_YELLOW << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                if(!success.empty()){
                    std::cout << ANSI_COLOR_GREEN << "Success:" << ANSI_COLOR_RESET << std::endl;
                    for(const auto& el : success){
                        std::cout << ANSI_COLOR_GREEN << el << ANSI_COLOR_RESET << std::endl;
                    }
                }
                S.save();
            }
        }
    }
    else if(std::string canName; getArgsName(canName, argv, argc) == 1){
        if(CS::Args::argcmp(argv, argc, "--restore") || CS::Args::argcmp(argv, argc, "-r")){
            if(std::string dateVal; CS::Args::argfcmp(dateVal, argv, argc, "--date") || CS::Args::argfcmp(dateVal, argv, argc, "-d")){
                int forceFlag = 0;
                if(CS::Args::argcmp(argv, argc, "--force") || CS::Args::argcmp(argv, argc, "-f")){
                    forceFlag = 1;
                }
                if(forceFlag){
                    std::cout << ANSI_COLOR_166 << "Undo restore action of " << canName << " from " << dateVal << " - Forced:" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_166 << "Undo restore action of " << canName << " from" << dateVal << ":" << ANSI_COLOR_RESET << std::endl;
                }
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                const uint64_t tstRaw = CS::Utility::str_to_timestamp<uint64_t>(dateVal);
                const uint64_t tst = getNearestResDate(S, canName, tstRaw);
                if(tst == 0){
                    std::cerr << "Fatal: Date not found." << std::endl;
                    return;
                }

                if(forceFlag){
                    for(const auto& proc : mgm.programs().at(canName).procNames){
                        if(CS::Process::killProcess(proc.c_str())){
                            std::cout << ANSI_COLOR_222 << "Killed " << proc << "." << ANSI_COLOR_RESET << std::endl;
                        }
                    }
                }

                if(!restoreSave(S, canName, tst)){
                    std::cerr << ANSI_COLOR_RED << "Failed to undo restore action of " << canName << "." << ANSI_COLOR_RESET << std::endl;
                    return;
                }
                else{
                    std::cout << ANSI_COLOR_GREEN << "Undo action successfull!" << ANSI_COLOR_RESET << std::endl;
                }
                
                S.save();
            }
            else{
                int forceFlag = 0;
                if(CS::Args::argcmp(argv, argc, "--force") || CS::Args::argcmp(argv, argc, "-f")){
                    forceFlag = 1;
                }
                if(forceFlag){
                    std::cout << ANSI_COLOR_166 << "Undo latest restore action of " << canName << " - Forced:" << ANSI_COLOR_RESET << std::endl;
                }
                else{
                    std::cout << ANSI_COLOR_166 << "Undo latest restore action of " << canName << ":" << ANSI_COLOR_RESET << std::endl;
                }
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                const uint64_t tst = getNearestResDate(S, canName, S.get_last_tst(canName));
                if(tst == 0){
                    std::cerr << "Fatal: No previous restore action to undo." << std::endl;
                    return;
                }

                if(forceFlag){
                    for(const auto& proc : mgm.programs().at(canName).procNames){
                        if(CS::Process::killProcess(proc.c_str())){
                            std::cout << ANSI_COLOR_222 << "Killed " << proc << "." << ANSI_COLOR_RESET << std::endl;
                        }
                    }
                }

                if(!restoreSave(S, canName, tst)){
                    std::cerr << ANSI_COLOR_RED << "Fatal: Failed to undo latest restore action of " << canName << "." << ANSI_COLOR_RESET << std::endl;
                    return;
                }
                else{
                    std::cout << ANSI_COLOR_GREEN << "Undo action successfull!" << ANSI_COLOR_RESET << std::endl;
                }

                S.save();
            }

        }
        else if(CS::Args::argcmp(argv, argc, "--save") || CS::Args::argcmp(argv, argc, "-s")){
            if(std::string dateVal; CS::Args::argfcmp(dateVal, argv, argc, "--date") || CS::Args::argfcmp(dateVal, argv, argc, "-d")){
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                std::cout << ANSI_COLOR_166 << "Undo save action of " << canName << " from " << dateVal << ":" << ANSI_COLOR_RESET << std::endl;
                const uint64_t tstRaw = CS::Utility::str_to_timestamp<uint64_t>(dateVal);
                const uint64_t tst = getNearestSaveDate(S, canName, tstRaw);
                if(tst == 0 || !S.find_save(canName, tst)){
                    std::cerr << "Fatal: Date not found." << std::endl;
                    return;
                }

                if(!removeSave(S, canName, tst)){
                    std::cerr << ANSI_COLOR_RED << "Failed to undo save action of " << canName << " from " << dateVal << ANSI_COLOR_RESET << std::endl;
                    return; 
                }
                else{
                    std::cout << ANSI_COLOR_GREEN << "Undo action successfull!" << ANSI_COLOR_RESET << std::endl;
                }

                S.save();
            }
            else{
                CS::Saves S(savesFile);
                if(!S.load()){
                    std::cerr << "Fatal: Nothing to undo." << std::endl;
                    return;
                }
                std::cout << ANSI_COLOR_166 << "Undo latest save action of " << canName << ":" << ANSI_COLOR_RESET << std::endl;
                const uint64_t tst = getNearestSaveDate(S, canName, S.get_last_tst(canName));
                if(tst == 0){
                    std::cerr << "Fatal: No previous save action to undo." << std::endl;
                    return;
                }

                if(!removeSave(S, canName, tst)){
                    std::cerr << ANSI_COLOR_RED << "Fatal: Failed to undo latest save action of " << canName << "." << ANSI_COLOR_RESET << std::endl;
                    return;
                }
                else{
                    std::cout << ANSI_COLOR_GREEN << "Undo action successfull!" << ANSI_COLOR_RESET << std::endl;
                }

                S.save();
            }
        }
        else{
            std::cerr << "Fatal: Unrecognized argument. See 'configsync --help'." << std::endl;
        }
    }
    else{
        std::cerr << "Fatal: Missing argument or value. See 'configsync --help'." << std::endl;
    }
}

inline void handleListOption(char* argv[], int argc){
    CS::Programs::Mgm mgm;
    if(argv[2] == NULL){
        std::cout << ANSI_COLOR_166 << "Supported Programs:" << ANSI_COLOR_RESET << std::endl;
        unsigned int i = 1;
        for(const auto& prog : mgm.get_supported()){
            std::cout << ANSI_COLOR_222 << i << ". " << prog << ANSI_COLOR_RESET << std::endl;
            i++;
        }
    }
    else{
        std::cerr << "Fatal: Unrecognized argument. See 'configsync --help'." << std::endl;
    }
}

int main(int argc, char* argv[]){   
    std::signal(SIGINT, exitSignalHandler);
    enableColors();
    
    pLocatFull = boost::dll::program_location().string();
    pLocat = boost::dll::program_location().parent_path().string();
    root = boost::dll::program_location().root_name().string();
    archiveDir = pLocat + "\\ConfigArchive";
    backupDir = pLocat + "\\PreRestoreBackups";
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
        std::cout << "usage: configsync [OPTIONS]... [PROGRAM]" << std::endl;
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
    else if(std::string(argv[1]) == "restore"){
        handleRestoreOption(argv, argc);
    }
    else if(std::string(argv[1]) == "show"){
        handleShowOption(argv, argc);
    }
    else if(std::string(argv[1]) == "status"){
        handleStatusOption(argv, argc);
    }
    else if(std::string(argv[1]) == "undo"){
        handleUndoOption(argv, argc);
    }
    else if(std::string(argv[1]) == "list"){
        handleListOption(argv, argc);
    }
    else{
        std::cerr << "Fatal: '" << argv[1] << "' is not a ConfigSync command.\n";
        return 1;
    }
    
    return 0;
}