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
#include <map>
#include "database.hpp"
#include "analyzer.hpp"

class synchronizer{
    private:
        const std::vector<std::string> programPaths;
        const std::string programName;
    public:
        synchronizer(const std::vector<std::string>& ppaths, const std::string& pname) : programPaths(ppaths), programName(pname) {}
        
        // hardcoded absolute program location //TODO: get program location automatically.
        const std::string absoluteProgramLocation = "C:\\Data\\TW\\Software\\Coding\\ConfigSync";


        std::string generate_UUID(){
            boost::uuids::random_generator generator;
            boost::uuids::uuid UUID = generator();
            return boost::uuids::to_string(UUID);
        }


        void copy_config(const std::string& archivePathAbs, const std::string& dateDir){ // archivePathAbs = programs directory containing the date directories
            // create date folder if it doesnt exist yet
            if(std::filesystem::is_empty(std::filesystem::path(archivePathAbs))){
                char out[11];
                std::time_t t=std::time(NULL);
                std::strftime(out, sizeof(out), "%Y-%m-%d", std::localtime(&t));
                std::filesystem::create_directory(absoluteProgramLocation + "\\" + "ConfigArchive\\" + programName + "\\" + out); // Create archive dir and the subdir "dateDir"
            }
            
            // PathDatabase location is inside the dateDir
            std::string databasePath = archivePathAbs + programName + "\\" + dateDir + "\\ConfigSync-PathDatabase.bin";
            std::map<std::string, std::string> pathMap;


            for(const auto& item : programPaths){
                const std::filesystem::path source = item;

                if(std::filesystem::is_directory(source)){
                    std::string destination = archivePathAbs + "\\" + dateDir + "\\" + source.filename().string();

                    try{
                        std::filesystem::copy(source, std::filesystem::path(destination), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                    }
                    catch(std::filesystem::filesystem_error& copyError){
                        throw std::runtime_error(copyError);
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
                        throw std::runtime_error(copyError);
                    }

                    // Store path pair in map
                    pathMap.insert(std::make_pair(item, destination));
                }
            }

            // Writes the path map to file
            database db(databasePath);
            db.storeStringMap(pathMap);
        }

        
        void backup_config_for_restore(const std::string& dirUUID){
            std::string backupDir = absoluteProgramLocation + "\\ConfigBackup\\" + programName;

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
            std::string databasePath = backupDir + "\\temp\\" + dirUUID;
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
        

        int restore_config(const std::string dateDir){
            // Check if programPaths exist
            for(const auto& item : programPaths){
                if(!std::filesystem::exists(std::filesystem::path(item))){
                    std::cerr << "Error: programs config path does not exist" << std::endl;
                    return 0;
                }
            }

            std::string backupDir = absoluteProgramLocation + "\\ConfigBackup\\" + programName;
            
            // Backup config
            std::string dirUUID = generate_UUID();
            
            // Clean up temp directory
            if(!std::filesystem::is_empty(std::filesystem::path(backupDir + "\\temp"))){
                for(const auto& item : std::filesystem::directory_iterator(backupDir + "\\temp")){
                    std::filesystem::rename(item, backupDir + "\\RecycleBin\\" + item.path().filename().string());
                }
            }
            // Backup to temp
            else{
                backup_config_for_restore(dirUUID);
            }


            // Get path database
            std::string databasePath = (absoluteProgramLocation + "\\ConfigArchive\\" + programName + "\\" + dateDir + "\\ConfigSync-PathDatabase.bin");
            database db(databasePath);
            std::map<std::string, std::string> pathMap;
            db.readStringMap(pathMap);


            // Replace config
            for(const auto& pair : pathMap){
                try{
                    std::filesystem::copy(pair.second, pair.first, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                }
                catch(std::filesystem::filesystem_error& copyError){
                    std::cerr << "Aborting synchronisation of config: Error during copying ( " << &copyError << ")" << std::endl;
                    std::cout << "Rebuilding config from backup..." << std::endl;
                    
                    const std::string databaseBackupPath = absoluteProgramLocation + "\\ConfigBackup\\" + programName + "\\temp\\" + dirUUID + "\\ConfigSync-PathDatabase.bin";

                    // Rebuild from backup
                    if(rebuild_from_backup(databaseBackupPath) != 1){
                        std::cerr << "Failed to rebuild from backup. (" << copyError.what() << "). Fatal, please verify that none of the selected programs components are missing. Potentially reinstall the affected application" << std::endl;  
                        return 0;
                    }
                    else{
                        std::cout << "Rebuild was successfull!" << std::endl;
                        return 1;
                    }
                }
            }
            
            return 1;
        }
};
#endif