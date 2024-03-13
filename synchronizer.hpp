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
#include <boost\filesystem.hpp>
#include <map>
#include <chrono>
#include <ranges>
#include "database.hpp"
#include "programs.hpp"
#include "organizer.hpp"
#include "ANSIcolor.hpp"
#include "CFGSExcept.hpp"


class synchronizer{
    private:
        const std::vector<std::string>& programPaths;
        const std::string& programName;
        const std::string& exeLocation;
        std::ofstream& logfile;
    public:
        synchronizer(const std::vector<std::string>& ppaths, const std::string& pname, const std::string& exeLocat, std::ofstream& logf) : programPaths(ppaths), programName(pname), exeLocation(exeLocat), logfile(logfile) {}
        

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
            char* out = (char *)malloc(11);
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

        
        /**
         * @brief Recursively copy a directory or a single file
         * @param source Source path
         * @param destination Destination path
         * @param map Optional pass by reference map of strings. When defined, source is inserted as key with destination as value.
         * @note Optional parameter requires a map inside a reference_wrapper container, created in part with std::ref(normMap). normMap is modified and can be used like normal afterwards.
         */
        static int recurse_copy(const std::filesystem::path& source, const std::string& destination,
                                std::optional<std::reference_wrapper<std::map<std::string, std::string>>> map = std::nullopt,
                                std::optional<std::reference_wrapper<std::vector<std::pair<std::string,std::string>>>> pp = std::nullopt){

            if(std::filesystem::is_directory(source)){
                for(const auto& entry : std::filesystem::directory_iterator(source)){
                    if(entry.is_directory()){
                        const std::string dst = destination + "\\" + entry.path().filename().string();
                        if(!std::filesystem::exists(dst)){
                            std::filesystem::create_directories(dst);
                        }
                        recurse_copy(entry.path(), dst, std::nullopt, pp);
                    }
                    else{
                        try{
                            if(!std::filesystem::exists(destination)){
                                std::filesystem::create_directories(destination);
                            }
                            const std::string dstPath = destination + "\\" + entry.path().filename().string(); // full destination
                            std::filesystem::copy_file(entry.path(), dstPath, std::filesystem::copy_options::overwrite_existing);
                            if(map.has_value()){
                                // Dereference the optional to access the map
                                auto& mapDeref = map->get();
                                mapDeref[entry.path().string()] = dstPath;
                            }
                            
                            if(pp.has_value()){
                                // Dereference the optional to access the vector
                                auto& ppDeref = pp->get();

                                ppDeref.push_back(std::make_pair(entry.path().string(), dstPath));
                            }
                        }
                        catch(const std::filesystem::filesystem_error& err){
                            throw cfgsexcept(err.what());
                            return 0;
                        }
                    }
                }
            }
            else{
                try{
                    if(!std::filesystem::exists(destination)){
                        std::filesystem::create_directories(destination);
                    }
                    const std::string dstPath = destination + "\\" + std::filesystem::path(source).filename().string(); // full destination
                    std::filesystem::copy_file(source, dstPath, std::filesystem::copy_options::overwrite_existing);
                    if(map.has_value()){
                        auto& mapDeref = map->get(); // Dereference the optional
                        mapDeref[source.string()] = dstPath; // Add element to map
                    }
                    if(pp.has_value()){
                        // Dereference the optional to access the vector
                        auto& ppDeref = pp->get();
                        ppDeref.push_back(std::make_pair(source.string(), dstPath));
                    }
                }
                catch(const std::filesystem::filesystem_error& err){
                    throw cfgsexcept(err.what());
                    return 0;
                }
            }

            
            return 1;
        }

        
        /**
         * @brief Remove files and directories recursively
         * @param path Target
         * @note Can delete read only files. Permissions are modified before removal.
         */
        static void recurse_remove(const std::filesystem::path& path){
            for(const auto& entry : std::filesystem::recursive_directory_iterator(path)){
                if(entry.is_directory()){
                    recurse_remove(entry);
                }
                else{
                    std::filesystem::permissions(entry, std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write);
                    std::filesystem::remove(entry);/*  */
                }
            }
        }


        /**
         * @brief Copy the program paths to the respective date directory inside the archive.
         * @param archivePathAbs This is the path to the directory in the archive, that contains all the date directories.
         * @param dateDir A specific date for the directory that the items are copied to.
         * @note Uses private members 'programPaths', 'programName' and 'exeLocation'.
         */
        int copy_config(const std::string& archivePathAbs, const std::string& dateDir){ // archivePathAbs = programs directory containing the date directories

            // create first date folder if it doesnt exist yet
            if(std::filesystem::is_empty(std::filesystem::path(archivePathAbs))){
                char* date = ymd_date_cstyle();

                std::filesystem::create_directory(exeLocation + "\\" + "ConfigArchive\\" + programName + "\\" + date); // Create archive dir and the subdir "dateDir"
            }
            


            const std::filesystem::path datePath = (archivePathAbs / std::filesystem::path(dateDir));
            if(!std::filesystem::exists(datePath)){ // If date dir doesnt exist, create it.
                std::cout << "archivePathAbs: " << archivePathAbs << std::endl;

                std::filesystem::create_directories(datePath);
            }
            else if(!std::filesystem::is_empty(datePath)){ // If exists but not empty, remove dir and recreate it. Handles cases where user runs sync command multiple times on the same day.

                recurse_remove(archivePathAbs);

                std::filesystem::create_directories(datePath);
            }
            std::cout << "archivePathAbs: " << archivePathAbs << std::endl;

            
  

            // hashbase location is inside the dateDir
            const std::string hashbasePath = archivePathAbs + "\\" + dateDir + "\\ConfigSync-Hashbase.csv";

            hashbase H; // Initialize hashbase
            std::optional<std::reference_wrapper<std::vector<std::pair<std::string,std::string>>>> ppRef = std::ref(H.pp);

            /* Copy Process */
            std::unordered_map<std::string, std::string> id;
            int groupFlag = 0;
            if(programconfig::has_groups(programName)){ // Check for groups
                programconfig pcfg(programName, exeLocation);
                id = pcfg.get_path_groups();
                groupFlag = 1;
            }
            else{
                groupFlag = 0;
            }

            for(const auto& item : programPaths){
                std::string groupName;
                if(std::filesystem::is_directory(item)){
                    if(groupFlag == 1 && id.contains(item)){ // Assign group or default name
                        groupName = id[item];
                    }
                    else{
                        groupName = "Directories";
                    }

                    const std::string destination = archivePathAbs + "\\" + dateDir + "\\" + groupName;
                    try{
                        recurse_copy(item, destination, std::nullopt, ppRef); // Copy and load into vector
                    }
                    catch(cfgsexcept& except){
                        std::cerr << except.what() << std::endl;
                    }
                }
                else{
                    if(groupFlag == 1 && id.contains(item)){ // Assign group or default name
                        groupName = id[item];
                    }
                    else{
                        groupName = "Single-Files";
                    }

                    const std::string destination = archivePathAbs + "\\" + dateDir + "\\" + groupName;
                    try{
                        recurse_copy(item, destination, std::nullopt, ppRef); // Copy and load into vector
                    }
                    catch(cfgsexcept& err){
                        std::cerr << err.what() << std::endl;
                    }
                }
            }


            database::storeHashbase(hashbasePath, H); // Write the hashbase to file

            /* Create username file */
            std::ofstream idfile(archivePathAbs + "\\" + dateDir + "\\id.bin");
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
            std::optional<std::reference_wrapper<std::map<std::string, std::string>>> mapRef = std::ref(pathMap);

            // Backup to UUID directory
            std::unordered_map<std::string, std::string> id;
            int groupFlag = 0;
            if(programconfig::has_groups(programName)){ // Check for groups
                programconfig pcfg(programName, exeLocation);
                id = pcfg.get_path_groups();
                groupFlag = 1;
            }
            else{
                groupFlag = 0;
            }
            for(const auto& item : programPaths){
                std::string groupName;

                if(std::filesystem::is_directory(item)){

                    if(groupFlag == 1 && id.contains(item)){
                        groupName = id[item];
                    }
                    else{
                        groupName = "Directories";
                    }

                    std::string destination = backupDir + "\\temp\\" + dirUUID + "\\" + groupName;
                    try{
                        recurse_copy(item, destination, mapRef);
                    }
                    catch(cfgsexcept& err){
                        std::cerr << err.what() << std::endl;
                    }
                }
                else{

                    if(groupFlag == 1 && id.contains(item)){
                        groupName = id[item];
                    }
                    else{
                        groupName = "Single-Files";
                    }

                    std::string destination = backupDir + "\\temp\\" + dirUUID + "\\" + groupName;
                    try{
                        recurse_copy(item, destination, mapRef);
                    }
                    catch(cfgsexcept& err){
                        std::cerr << err.what() << std::endl;
                    }
                }
            }

            // Write path map to file

            database db(databasePath);
            db.storeStringMap(pathMap);
        }


        static int rebuild_from_backup(const std::string databasePath){
            database db(databasePath);

            std::map<std::string, std::string> pathMap;
            db.readStringMap(pathMap);
            
            for(const auto& pair : pathMap){
                try{
                   recurse_copy(std::filesystem::path(pair.second), pair.first); 
                }
                catch(cfgsexcept& error){
                    std::cerr << error.what() << std::endl;
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
         * @brief Convert username of paths as elements of a vector of a pair.
         * @param vec Vector of type <std::string, std::string> containing paths.
         * @param newUser The new username.
         */
        static void transform_pathvector_new_username(std::vector<std::pair<std::string, std::string>>& vec, const std::string& newUser){
            
            const boost::regex pattern("C:\\\\Users\\\\");
            const boost::regex repPattern("(?<=C:\\\\Users\\\\)(.+?)\\\\");
            
            for(auto & pair : vec){
                
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

                        /* Replace pair element */
                        pair = std::make_pair(newKey, newValue);

                    }
                    else{ // Replace only key when value did not contain username
                        pair = std::make_pair(newKey, pair.second);
                    }
                }

                /* Check value when key did not contain username */
                else if(boost::regex_search(pair.second, valueMatches, pattern)){
                    newValue = boost::regex_replace(pair.second, repPattern, newUser);

                    /* Replace value  */
                    pair = std::make_pair(pair.first, newValue);
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
            const std::string backupDir = exeLocation + "\\ConfigBackup\\" + programName;
            const std::string dirUUID = generate_UUID();
            std::map<unsigned long long, std::string> recycleMap; // Stores recycleBin items with timestamps

            if(!std::filesystem::is_empty(std::filesystem::path(backupDir + "\\temp"))){ // Clean up temp directory
                database db(backupDir + "\\RecycleBin\\RecycleMap.bin"); // Inside respective programs RecycleBin dir

                for(const auto& item : std::filesystem::directory_iterator(backupDir + "\\temp")){
                    const std::string newPath = backupDir + "\\RecycleBin\\" + item.path().filename().string();
                    std::filesystem::rename(item, newPath); // Move item to recyclebin
                    timestamp_objects(recycleMap, newPath); // Timestamp and load into map                                       
                }

                // Clean recyclebin
                const std::string mapPath = backupDir + "\\RecycleBin\\RecycleMap.bin";
                organizer janitor;
                janitor.recyclebin_cleaner(recyclebinlimit, recycleMap, mapPath); // Clean recycle bin and store recycle map as file
            }

            std::cout << ANSI_COLOR_YELLOW << "Backing up program config, in case of plan B..." << ANSI_COLOR_RESET << std::endl; // Verbose
            backup_config_for_restore(dirUUID); // Backup to temp directory

            // Get path database from ConfigArchive
            const std::string databasePath = (exeLocation + "\\ConfigArchive\\" + programName + "\\" + dateDir + "\\ConfigSync-Hashbase.csv");
            hashbase H;
            database::readHashbase(databasePath, H, logfile);


            /* Get username file */
            std::string idPath = exeLocation + "\\ConfigArchive\\" + programName + "\\" + dateDir + "\\id.bin";
            std::ifstream idFile(idPath);
            std::string id = database::read_lenght_prefix_encoded_string(idFile);

            if(id != programconfig::get_username()){ // Check if username is still valid
                transform_pathvector_new_username(H.pp, id); // Update username
            }


            
            // Replace config
            for(const auto& pair : H.pp){
                try{
                    recurse_copy(std::filesystem::path(pair.second), pair.first);
                }
                catch(cfgsexcept& copyError){ // Error. Breaks for loop 
                    std::cerr << "Aborting synchronisation: ( " << copyError.what() << ")" << std::endl;
                    std::cout << "Plan B: Rebuilding from backup..." << std::endl;
                    
                    const std::string databaseBackupPath = exeLocation + "\\ConfigBackup\\" + programName + "\\temp\\" + dirUUID + "\\ConfigSync-PathDatabase.bin";

                    // Rebuild from backup
                    if(rebuild_from_backup(databaseBackupPath) != 1){
                        std::cerr << ANSI_COLOR_RED << "Fatal: Failed to rebuild from backup.\n Please verify that none of the selected programs components are missing or corrupted." << ANSI_COLOR_RESET << std::endl;
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