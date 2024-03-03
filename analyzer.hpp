#ifndef ANALYZER_HPP
#define ANALYZER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <fstream>


class analyzer{
    private:
        const std::vector<std::string> programPaths;
        const std::string programName;
        const std::string exeLocation;
    public:
        analyzer(const std::vector<std::string>& path, const std::string& name, const std::string& exeLocat) : programPaths(path), programName(name), exeLocation(exeLocat) {}


        const std::string archivePath = exeLocation + "\\ConfigSync\\ConfigArchive\\" + programName;

        static void recurse_scanner(const std::filesystem::path& dirPath, std::vector<std::string>& itemVector){ // Self referencing function. itemVector parameter passed in as reference, to modify original object directly.
            for(const auto& entry : std::filesystem::directory_iterator(dirPath)){
                if(entry.is_directory()){
                    recurse_scanner(entry.path(), itemVector);
                }
                else{
                    itemVector.push_back(entry.path().string());
                }
            }
        }
        

        static int is_identical_filenames(const std::vector<std::string>& base, const std::vector<std::string>& reference){
            int i = 0;
            for(const auto& entry : base){
                const std::filesystem::path entry_fs = entry;
                const std::filesystem::path reference_fs = reference[i];

                if(entry_fs.filename() != reference_fs.filename()){
                    return 0;
                }

                i++;
            }
            return 1;
        }


        static int is_identical_bytes(const std::string& path1, const std::string& path2){
            std::fstream file1Binary(path1, std::iostream::binary);
            std::fstream file2Binary(path2, std::iostream::binary);

            if(!file1Binary.is_open() || !file2Binary.is_open()){
                file1Binary.close();
                file2Binary.close();
                throw std::runtime_error("Error opening files - Error-Code 52");
            }
            
            char byte1, byte2;
            while(true){
                byte1 = file1Binary.get();
                byte2 = file2Binary.get();

                if(byte1 != byte2){
                    file1Binary.close();
                    file2Binary.close();
                    return 0;
                }

                if(file1Binary.eof() && file2Binary.eof()){
                    file1Binary.close();
                    file2Binary.close();
                    return 1;
                }
            }
        }


        std::string get_newest_backup_path(){
            const std::filesystem::path archivePathFs = archivePath;

            std::vector<std::string> dateDirs;
            for(const auto& item : std::filesystem::directory_iterator(archivePathFs)){
                if(item.is_directory()){
                    dateDirs.push_back(item.path().generic_string());
                }
            }
            std::sort(dateDirs.rbegin(), dateDirs.rend());

            return dateDirs[0];
        }

        
        void get_config_items_saved(std::vector<std::string>& configItems){
            const std::string lastSavedDir = get_newest_backup_path();
            const std::filesystem::path configPathSaved = archivePath + "\\" + lastSavedDir;
            
            recurse_scanner(configPathSaved, configItems);
        
        }
        

        void get_config_items_current(std::vector<std::string>& configItems){
            for(const auto& item : programPaths){
                recurse_scanner(item, configItems);
            }
        }    


        void sortby_filename(std::vector<std::string>& filenames){
            std::sort(filenames.begin(), filenames.end(), [this](const std::string& path1, const std::string& path2){
                std::string filename1 = std::filesystem::path(path1).filename().generic_string();
                std::string filename2 = std::filesystem::path(path2).filename().generic_string();
                return filename1 < filename2;
            });
        }


        int is_identical_config(){ // Main function of this class

            // Get saved config items
            std::vector<std::string> configItemsSaved;
            get_config_items_saved(configItemsSaved);
            sortby_filename(configItemsSaved);
            
            // Get ProgramPaths
            std::vector<std::string> configItemsCurrent;
            get_config_items_current(configItemsCurrent);
            sortby_filename(configItemsCurrent);
            
            // Compare vectors
            if(is_identical_filenames(configItemsSaved, configItemsCurrent)){
                int i = 0;
                for(const auto& item : configItemsSaved){
                    
                    if(!is_identical_bytes(item, configItemsCurrent[i])){
                        return 0; // config changed
                    }
                    
                    i++;
                }
            }

            return 1; // config did not change
        }


        // Get all saves for the program. Iterates over archivePath and loads all directories into the vector.
        void get_all_saves(std::vector<std::string>& storageVector){

            for(const auto & item : std::filesystem::directory_iterator(archivePath)){
                if(item.is_directory()){
                    storageVector.push_back(item.path().filename().string());
                }
            }
        }
};
#endif