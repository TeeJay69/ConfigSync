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

class synchronizer{
    private:
        const std::vector<std::string> programPaths;
        const std::string programName;
    public:
        synchronizer(const std::vector<std::string>& ppaths, const std::string& pname) : programPaths(ppaths), programName(pname) {}
        
        std::string generate_UUID(){
            boost::uuids::random_generator generator;
            boost::uuids::uuid UUID = generator();
            return boost::uuids::to_string(UUID);
        }

        void copy_config(const std::string& archivePath, const std::string& dateDir){ // archivePath = programs directory containing the date directories
            // create date folder if it doesnt exist yet
            if(std::filesystem::is_empty(std::filesystem::path(archivePath))){
                char out[11];
                std::time_t t=std::time(NULL);
                std::strftime(out, sizeof(out), "%Y-%m-%d", std::localtime(&t));
                std::filesystem::create_directory("ConfigArchive\\" + programName + "\\" + out); // dateDir
            }
            
            // PathDatabase location is inside the dateDir
            std::string databasePath = archivePath + programName + "\\" + dateDir + "\\ConfigSync-PathDatabase.bin";
            std::map<std::string, std::string> pathMap;


            for(const auto& item : programPaths){
                const std::filesystem::path source = item;

                if(std::filesystem::is_directory(source)){
                    std::string destination = archivePath + "\\" + dateDir + "\\" + source.filename().string();

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
                    std::string destination = archivePath + "\\" + dateDir + "\\" + sourceParentDir + "\\" + source.filename().string();

                    try{
                        std::filesystem::create_directories(archivePath + "\\" + dateDir + "\\" + sourceParentDir);
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
            std::string backupDir = "ConfigBackup\\" + programName;

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

            // Backup to UUID directory
            for(const auto& item : programPaths){
                const std::filesystem::path source = item;
                
                if(std::filesystem::is_directory(source)){
                    try{
                        std::filesystem::copy(source, backupDir + "\\temp\\" + dirUUID + "\\" + source.filename().string(), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                    }
                    catch(std::filesystem::filesystem_error& copyError){
                        throw std::runtime_error(copyError);
                   }
                }
                else{
                    try{
                        std::string sourceParentDir = source.parent_path().filename().string();
                        std::filesystem::create_directories(backupDir + "\\temp\\" + dirUUID + "\\" + sourceParentDir);
                        std::filesystem::copy(source, backupDir + "\\temp\\" + dirUUID + "\\" + sourceParentDir + "\\" + source.filename().string());
                    }
                    catch(std::filesystem::filesystem_error& copyError){
                        throw std::runtime_error(copyError);
                    }
                }
            }
        }

        
        int restore_config(){
            // Check if programPaths exist
            for(const auto& item : programPaths){
                if(!std::filesystem::exists(std::filesystem::path(item))){
                    std::cerr << "Failed to restore config. Error: programs config path does not exist";
                    return 0;
                }
            }

            std::string backupDir = "ConfigBackup\\" + programName;
            // Clean up temp directory
            if(!std::filesystem::is_empty(std::filesystem::path(backupDir + "\\temp"))){
                for(const auto& item : std::filesystem::directory_iterator(backupDir + "\\temp")){
                    std::filesystem::rename(item, backupDir + "\\RecycleBin\\" + item.path().filename().string());
                }
            }
            // Backup to temp
            else{
                std::string dirUUID = generate_UUID();
                backup_config(dirUUID);
            }
        }
};
#endif