#ifndef SYNCHRONIZER_HPP
#define SYNCHRONIZER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>
#include <boost\uuid\uuid_io.hpp>
#include <boost\regex.hpp>
#include <map>
#include <chrono>
#include "database.hpp"
#include "programs.hpp"
#include "ANSIcolor.hpp"


class synchronizer{
    private:
        const std::vector<std::string> programPaths;
        const std::string programName;
        const std::string exeLocation;
    public:
        synchronizer(const std::vector<std::string>& ppaths, const std::string& pname, const std::string& exeLocat) : programPaths(ppaths), programName(pname), exeLocation(exeLocat) {}
        

        static std::string generate_UUID(){
            boost::uuids::random_generator generator;
            boost::uuids::uuid UUID = generator();
            return boost::uuids::to_string(UUID);
        }

        
        static std::string ymd_date(){
            const std::chrono::time_point now(std::chrono::system_clock::now());
            const std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(now));

            std::stringstream ss;
            ss << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year()) << '-'
            << std::setw(2) << static_cast<unsigned>(ymd.month()) << '-'
            << std::setw(2) << static_cast<unsigned>(ymd.day());

            return ss.str();
        }        
        
        
        static char* ymd_date_cstyle(){
            char out[11];
            std::time_t t=std::time(NULL);
            std::strftime(out, sizeof(out), "%Y-%m-%d", std::localtime(&t));
            
            return out;
        }

        
        static void timestamp_objects(std::map<unsigned long long, std::string>& map, const std::string& strValue){
            std::chrono::time_point<std::chrono::system_clock> timePoint;
            timePoint = std::chrono::system_clock::now();
            unsigned long long t = std::chrono::system_clock::to_time_t(timePoint);
            
            map[t] = strValue;
        }


        // Saves a programs config to the respective archive.
        // Date dir creation handled automatically.
        int copy_config(const std::string& archivePathAbs, const std::string& dateDir){ // archivePathAbs = programs directory containing the date directories

            // create first date folder if it doesnt exist yet
            if(std::filesystem::is_empty(std::filesystem::path(archivePathAbs))){

                char* date = ymd_date_cstyle();
                
                std::filesystem::create_directory(exeLocation + "\\" + "ConfigArchive\\" + programName + "\\" + date); // Create archive dir and the subdir "dateDir"
            }
            

            if(!std::filesystem::exists(archivePathAbs + "\\" + dateDir)){ // If date dir doesnt exist, create it.
                std::filesystem::create_directories(archivePathAbs + "\\" + dateDir);
            }
            else if(!std::filesystem::is_empty(archivePathAbs + "\\" + dateDir)){ // If exists but not empty, remove dir and recreate it. Handles cases where user runs sync command multiple times on the same day.
                std::filesystem::remove(archivePathAbs + "\\" + dateDir);
                std::filesystem::create_directories(archivePathAbs + "\\" + dateDir);
            }
            


            // PathDatabase location is inside the dateDir
            std::string databasePath = archivePathAbs + "\\" + programName + "\\" + dateDir + "\\ConfigSync-PathDatabase.bin";
            std::map<std::string, std::string> pathMap;

            // Copy process
            for(const auto& item : programPaths){
                const std::filesystem::path source = item;

                if(std::filesystem::is_directory(source)){
                    std::string destination = archivePathAbs + "\\" + dateDir + "\\" + source.filename().string();

                    try{
                        std::filesystem::copy(source, std::filesystem::path(destination), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                    }
                    catch(std::filesystem::filesystem_error& copyError){
                        return 0; // throw std::runtime_error(copyError);
                    }

                    // Store path pair in map
                    pathMap.insert(std::make_pair(item, destination));
                }
                else{
                    std::string sourceParentDir = source.parent_path().filename().string();
                    std::string destination = archivePathAbs + "\\" + dateDir + "\\" + sourceParentDir + "\\" + source.filename().string();

                    try{
                        std::filesystem::create_directories(archivePathAbs + "\\" + dateDir + "\\" + sourceParentDir);
                        std::filesystem::copy(source, std::filesystem::path(destination), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                    }
                    catch(std::filesystem::filesystem_error& copyError){
                        return 0; // throw std::runtime_error(copyError);
                    }

                    // Store path pair in map
                    pathMap.insert(std::make_pair(item, destination));
                }
            }

            // Writes the path map to file
            database db(databasePath);
            db.storeStringMap(pathMap);

            /* Create username file */
            std::ofstream idfile(archivePathAbs + "\\" + programName + "\\" + dateDir + "\\id.bin");
            std::string username = programconfig::get_username();
            database::encodeStringWithPrefix(idfile, username);

            
            return 1;
        }

        
        void backup_config_for_restore(const std::string& dirUUID){
            std::string backupDir = exeLocation + "\\ConfigBackup\\" + programName;

            if(std::filesystem::is_empty(backupDir)){
                std::filesystem::create_directories(backupDir + "\\temp");
                std::filesystem::create_directories(backupDir + "\\RecycleBin");
            }
            
            // Create UUID directory
            try{
                std::filesystem::create_directory(backupDir + "\\temp\\" + dirUUID);
            }
            catch(std::filesystem::filesystem_error& errorCode){
                throw std::runtime_error(errorCode);
            }

            // databasePath inside temp UUID dir 
            std::string databasePath = backupDir + "\\temp\\" + dirUUID + "\\" + "ConfigSync-PathDatabase.bin";
            std::map<std::string, std::string> pathMap;

            // Backup to UUID directory
            for(const auto& item : programPaths){
                const std::filesystem::path source = item;
                
                if(std::filesystem::is_directory(source)){
                    std::string destination = backupDir + "\\temp\\" + dirUUID + "\\" + source.filename().string();

                    try{
                        std::filesystem::copy(source, destination, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                    }
                    catch(std::filesystem::filesystem_error& copyError){
                        throw std::runtime_error(copyError);
                   }

                    // Store path pairs
                   pathMap.insert(std::make_pair(item, destination));
                }
                else{
                    std::string sourceParentDir = source.parent_path().filename().string();
                    std::string destination = backupDir + "\\temp\\" + dirUUID + "\\" + sourceParentDir + "\\" + source.filename().string();
                    
                    try{
                        std::filesystem::create_directories(backupDir + "\\temp\\" + dirUUID + "\\" + sourceParentDir);
                        std::filesystem::copy(source, destination, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                    }
                    catch(std::filesystem::filesystem_error& copyError){
                        throw std::runtime_error(copyError);
                    }

                    // Store path pairs
                    pathMap.insert(std::make_pair(item, destination));
                }
            }

            // Write path map to file
            database db(databasePath);
            db.storeStringMap(pathMap);
        }


        int rebuild_from_backup(const std::string databasePath){
            database db(databasePath);

            std::map<std::string, std::string> pathMap;
            db.readStringMap(pathMap);
            
            for(const auto& pair : pathMap){
                try{
                    std::filesystem::copy(pair.second, pair.first, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                }
                catch(std::filesystem::filesystem_error& copyError){
                    return 0;
                }
            }
            
            return 1;
        }
        

        /**
         * @brief Convert username of paths as elements of a map.
         * @param map Map of type <std::string, std::string> containing paths.
         * @param newUser The new username.
         */
        static void transform_pathmap_new_username(std::map<std::string, std::string>& map, const std::string& newUser){
            
            const boost::regex pattern("C:\\\\Users\\\\");
            const boost::regex repPattern("(?<=C:\\\\Users\\\\)(.+?)\\\\");
            
            for(const auto & pair : map){
                
                auto keyMatches = boost::smatch{};
                auto valueMatches = boost::smatch{};

                std::string newKey;
                std::string newValue;

                
                /* Check key */
                if(boost::regex_search(pair.first, keyMatches, pattern)){
                    
                    newKey = boost::regex_replace(pair.first, repPattern, newUser);

                    /* Search value */
                    if(boost::regex_search(pair.second, valueMatches, pattern)){
                        
                        newValue = boost::regex_replace(pair.second, repPattern, newUser);    

                        /* Replace map element */
                        map.erase(pair.first); // Remove key value pair
                        map.insert(std::make_pair(newKey, newValue));
                    }
                    else{ // Replace key when value did not contain username

                        map.erase(pair.first);
                        map.insert(std::make_pair(newKey, pair.second));
                    }
                }

                /* Check value when key did not contain username */
                else if(boost::regex_search(pair.second, valueMatches, pattern)){
                    newValue = boost::regex_replace(pair.second, repPattern, newUser);

                    /* Replace value  */
                    map[pair.first] = newValue;
                }
            }
        }


        /**
         * @brief Replace a programs config with a save from the config archive.
         * @param dateDir Date to identify the save.
         * @param recyclebinlimit Amount of backups to keep of a single programs config, resulting from the restore process (backup_config_for_restore function).
         * @note The recycle bin management occurs automatically.
         */
        int restore_config(const std::string dateDir, const int& recyclebinlimit){ // Main function for restoring. Uses generate_UUID, timestamp_objects, backup_config_for_restore, backup_config_for_restore, rebuild_from_backup
            // Check if programPaths exist
            for(const auto& item : programPaths){
                if(!std::filesystem::exists(std::filesystem::path(item))){
                    std::cerr << "Error: programs config path does not exist" << std::endl;
                    return 0;
                }
            }

            /* Create backup of current config */
            std::string backupDir = exeLocation + "\\ConfigBackup\\" + programName;
            std::string dirUUID = generate_UUID();
            std::map<unsigned long long, std::string> recycleMap; // Stores recycleBin items with timestamps

            if(!std::filesystem::is_empty(std::filesystem::path(backupDir + "\\temp"))){ // Clean up temp directory
                database db(backupDir + "\\RecycleBin\\RecycleMap.bin"); // Inside respective programs RecycleBin dir
                
                for(const auto& item : std::filesystem::directory_iterator(backupDir + "\\temp")){
                    const std::string newPath = backupDir + "\\RecycleBin\\" + item.path().filename().string();
                    std::filesystem::rename(item, newPath); // Move item to recyclebin
                    timestamp_objects(recycleMap, newPath); // Timestamp and load into map                                       
                }

                // Clean recyclebin
                std::string mapPath = backupDir + "\\RecycleBin\\RecycleMap.bin";
                organizer janitor;
                janitor.recyclebin_cleaner(recyclebinlimit, recycleMap, mapPath); // Clean recycle bin and store recycle map as file


                //// db.storeIntMap(recycleMap); // Store recycle map as file
            }

            std::cout << ANSI_COLOR_YELLOW << "Backing up program config, in case of plan B..." << ANSI_COLOR_RESET << std::endl; // Verbose
            backup_config_for_restore(dirUUID); // Backup to temp directory

            // Get path database from ConfigArchive
            std::string databasePath = (exeLocation + "\\ConfigArchive\\" + programName + "\\" + dateDir + "\\ConfigSync-PathDatabase.bin");
            database db(databasePath);
            std::map<std::string, std::string> pathMap;
            db.readStringMap(pathMap);


            /* Get username file */
            std::string idPath = exeLocation + "\\ConfigArchive\\" + programName + "\\" + dateDir + "\\id.bin";
            std::ifstream idFile(idPath);
            std::string id = database::read_lenght_prefix_encoded_string(idFile);

            if(id != programconfig::get_username()){ // Check if username is still valid
                transform_pathmap_new_username(pathMap, id); // Update username
            }


            
            // Replace config
            for(const auto& pair : pathMap){
                try{
                    std::filesystem::copy(pair.second, pair.first, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                }
                catch(std::filesystem::filesystem_error& copyError){ // Error. Breaks for loop 
                    std::cerr << "Aborting synchronisation: Error during copying ( " << &copyError << ")" << std::endl;
                    std::cout << "Plan B: Rebuilding from backup..." << std::endl;
                    
                    const std::string databaseBackupPath = exeLocation + "\\ConfigBackup\\" + programName + "\\temp\\" + dirUUID + "\\ConfigSync-PathDatabase.bin";

                    // Rebuild from backup
                    if(rebuild_from_backup(databaseBackupPath) != 1){
                        std::cerr << ANSI_COLOR_RED << "Fatal: Failed to rebuild from backup. (" << copyError.what() << "). Please verify that none of the selected programs components are missing or corrupted." << ANSI_COLOR_RESET << std::endl;
                        std::cerr << "You may have to reinstall the affected application" << std::endl;  
                        return 0;
                    }
                    else{
                        std::cout << ANSI_COLOR_YELLOW << "Rebuild was successfull!" << ANSI_COLOR_RESET << std::endl;
                        return 0; // The main goal did not succeed. We have to return 0.    
                    }
                }
            }
            
            return 1;
        }
};
#endif