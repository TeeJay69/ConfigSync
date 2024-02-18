#ifndef SYNCHRONIZER_HPP
#define SYNCHRONIZER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

class synchronizer{
    private:
        const std::vector<std::string> programPaths;
        const std::string backupPath;
    public:
        synchronizer(const std::vector<std::string>& path1, const std::string path2) : programPaths(path1), backupPath(path2) {}

        void copy_config(){
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