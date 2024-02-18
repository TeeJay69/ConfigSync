#ifndef SYNCHRONIZER_HPP
#define SYNCHRONIZER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>

class synchronizer{
    private:
        const std::string programName;
        const std::vector<std::string> programPaths;
        const std::string backupPath;
    public:
        synchronizer(const std::string name, const std::vector<std::string>& path1, const std::string path2) : programName(name), programPaths(path1), backupPath(path2) {}
        
        void copy_config(){
            if(backupPath.empty()){
                char out[11];
                std::time_t t=std::time(NULL);
                std::strftime(out, sizeof(out), "%Y-%m-%d", std::localtime(&t));
                std::filesystem::create_directory("ConfigArchive\\" + programName + "\\" + out);
            }
            for(const auto& item : programPaths){
                const std::filesystem::path source = item;

                try{
                    std::filesystem::copy(source, backupPath + "\\" + source.parent_path().string() + "\\" + source.filename().string());
                }
                catch(std::filesystem::filesystem_error& copyError){
                    throw std::runtime_error(copyError);
                }
            }
        }
};
#endif