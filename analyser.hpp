#ifndef PROGRAMS_HPP
#define PROGRAMS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

class analyser{
    private:
        const std::vector<std::string> programPaths;
        const std::string programName;
    public:
        analyser(const std::vector<std::string>& path, const std::string& name) : programPaths(path), programName(name) {}

        const std::string backupDir = "C:\\Data\\TW\\Software\\Coding\\ConfigSync\\ConfigArchive\\" + programName;
        std::string getLastBackupPath(){
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

        void recurseScanner(const std::filesystem::path& dirPath, std::vector<std::string>& itemVector){ // Self referencing function. itemVector parameter passed in as reference, to modify original object directly.
            for(const auto& entry : std::filesystem::directory_iterator(dirPath)){
                if(entry.is_directory()){
                    recurseScanner(entry.path(), itemVector);
                }
                else{
                    itemVector.push_back(entry.path().string());
                }
            }
        }

        std::string checkDiff(){
            const std::string lastSavedDir = getLastBackupPath();
            const std::filesystem::path savedConfigPath = backupDir + "\\" + lastSavedDir;
            
            // Scan saved config directory recursively
            std::vector<std::string> savedConfigItems;
            recurseScanner(savedConfigPath, savedConfigItems);

            
            // Scan each item of ProgramPaths recursively
            std::vector<std::string> currentConfigItems; 
            for(const auto& item : programPaths){
                recurseScanner(item, currentConfigItems);
            }

            // Compare vectors
            // Load last_write_time in multilevel vector
        }
};
#endif