#ifndef PROGRAMS_HPP
#define PROGRAMS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <fstream>

class analyser{
    private:
        const std::vector<std::string> programPaths;
        const std::string programName;
    public:
        analyser(const std::vector<std::string>& path, const std::string& name) : programPaths(path), programName(name) {}

        const std::string backupDir = "C:\\Data\\TW\\Software\\Coding\\ConfigSync\\ConfigArchive\\" + programName;
        

        void recurse_scanner(const std::filesystem::path& dirPath, std::vector<std::string>& itemVector){ // Self referencing function. itemVector parameter passed in as reference, to modify original object directly.
            for(const auto& entry : std::filesystem::directory_iterator(dirPath)){
                if(entry.is_directory()){
                    recurse_scanner(entry.path(), itemVector);
                }
                else{
                    itemVector.push_back(entry.path().string());
                }
            }
        }
        
        int is_identical_filenames(const std::vector<std::string>& base, const std::vector<std::string>& reference){
            int i = 0;
            for(const auto& entry : base){
                std::filesystem::path entry_fs = entry;
                std::filesystem::path reference_fs = reference[i];

                if(entry_fs.filename() != reference_fs.filename()){
                    return 0;
                }

                i++;
            }
            return 1;
        }

        int byte_comparison();

        std::string get_last_backup_path(){
            const std::filesystem::path backupDirFs = backupDir;

            std::vector<std::string> dateDirs;
            for(const auto& item : std::filesystem::directory_iterator(backupDirFs)){
                if(item.is_directory()){
                    dateDirs.push_back(item.path().generic_string());
                }
            }
            std::sort(dateDirs.rbegin(), dateDirs.rend());

            return dateDirs[0];
        }

        std::string check_diff(){
            const std::string lastSavedDir = get_last_backup_path();
            const std::filesystem::path savedConfigPath = backupDir + "\\" + lastSavedDir;
            
            // Scan saved config directory recursively
            std::vector<std::string> savedConfigItems;
            recurse_scanner(savedConfigPath, savedConfigItems);

            
            // Scan each item of ProgramPaths recursively
            std::vector<std::string> currentConfigItems; 
            for(const auto& item : programPaths){
                recurse_scanner(item, currentConfigItems);
            }

            // Compare vectors
            if(is_identical_filenames(savedConfigItems, currentConfigItems)){
                
            }
            else{
                std::string matchError = "Error: (checkDiff) entries do not match!"; // For debugging only
                throw std::runtime_error(matchError);

                // --> structure changed, new save config
            }

            // Load last_write_time in multilevel vector
        }
};
#endif