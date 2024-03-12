#ifndef ANALYZER_HPP
#define ANALYZER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <openssl\md5.h>
#include <map>
#include <sstream>
#include <utility>
#include "database.hpp"
#include "programs.hpp"
#include "organizer.hpp"
#include "ANSIcolor.hpp"
#include "CFGSExcept.hpp"
#include "hashbase.hpp"


class analyzer{
    private:
        const std::vector<std::string> programPaths;
        const std::string programName;
        const std::string exeLocation;
    public:
        analyzer(const std::vector<std::string>& path, const std::string& name, const std::string& exeLocat) : programPaths(path), programName(name), exeLocation(exeLocat) {}


        const std::string archivePath = exeLocation + "\\ConfigArchive\\" + programName;

        /**
         * @brief Iterate recursively over a directory and pull all items into vector.
         * @param dirPath Target path as filesystem object
         * @param itemVector Vector of strings that the items get loaded into.
         * @note Self referencing function. Original object is modified.
         */
        static void recurse_scanner(const std::filesystem::path& dirPath, std::vector<std::string>& itemVector){
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
            std::fstream file1Binary(path1, std::ios::in);
            std::fstream file2Binary(path2, std::ios::in);
            
            if(!file1Binary.is_open() || !file2Binary.is_open()){
                file1Binary.close();
                file2Binary.close();
                std::cerr << "Error opening files: [" << __FILE__ << "] [" << __LINE__ << "]\n";
                throw std::runtime_error("See above");
            }
            
            char byte1, byte2;
            while(true){

                byte1 = file1Binary.get();
                byte2 = file2Binary.get();

                if(byte1 != byte2){
                    file1Binary.close();
                    file2Binary.close();
                    std::cout << "is_identical_bytes function: Files not identical!\n";
                    return 0;
                }

                if(file1Binary.eof() && file2Binary.eof()){
                    file1Binary.close();
                    file2Binary.close();
                    return 1;
                }
            }
        }

        // Check if a programs archive is empty
        int is_archive_empty(){
            std::cout << "Debug a83\n";
            if(std::filesystem::is_empty(archivePath)){
                std::cout << "Debug a85\n";
                return 1;
            }
            
            std::cout << "Debug a89\n";
            return 0;
        }


        std::string get_newest_backup_path(){
            const std::filesystem::path archivePathFs = archivePath;

            std::vector<std::string> dateDirs;
            for(const auto& item : std::filesystem::directory_iterator(archivePathFs)){
                if(item.is_directory()){
                    std::filesystem::path itemPath(item); // making copy due to make_preferred needing non const
                    dateDirs.push_back(itemPath.make_preferred().string());
                }
            }
            std::sort(dateDirs.rbegin(), dateDirs.rend());

            return dateDirs[0];
        }

        
        void get_config_items_saved(std::vector<std::string>& configItems){
            const std::string lastSavedDir = get_newest_backup_path();

            if(!std::filesystem::exists(lastSavedDir)){
                std::cerr << "The last save directory does not exist. " << __FILE__ << " " << __LINE__ << std::endl;
                throw cfgsexcept("This should never happen!");
            }

            recurse_scanner(std::filesystem::path(lastSavedDir), configItems);        
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
            std::cout << "items saved 0,1: " << configItemsSaved[0] << "  1: " << configItemsSaved[1] << std::endl;
            
            // Get ProgramPaths
            std::vector<std::string> configItemsCurrent;
            get_config_items_current(configItemsCurrent);
            sortby_filename(configItemsCurrent);
            std::cout << "items current 0,1: " << configItemsCurrent[0] << "  1: " << configItemsCurrent[1] << std::endl;
            
            // Compare vectors
            if(is_identical_filenames(configItemsSaved, configItemsCurrent)){
                int i = 0;
                for(const auto& item : configItemsSaved){
                    
                    if(!is_identical_bytes(item, configItemsCurrent[i])){
                        std::cout << "FILES NOT IDENTICAL!!!: " << item << configItemsCurrent[i] << std::endl;
                        return 0; // config changed
                    }
                    
                    i++;
                }
            }
            else{
                std::cerr << "this is a problem!" << std::endl;
                return 0;
            }

            std::cout << "Debug a169\n";
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

        std::string get_md5hash(const std::string& fname){ 
            char buffer[4096]; 
            unsigned char digest[MD5_DIGEST_LENGTH]; 

            std::stringstream ss; 
            std::string md5string; 

            std::ifstream ifs(fname, std::ifstream::binary); 

            MD5_CTX md5Context; 

            MD5_Init(&md5Context); 


            while(ifs.good()){ 
                ifs.read(buffer, sizeof(buffer));
                MD5_Update(&md5Context, buffer, ifs.gcount());
            } 

            ifs.close(); 

            int res = MD5_Final(digest, &md5Context); 
                
                if( res == 0 ) // hash failed 
                return {};   // or raise an exception 

            // set up stringstream format 
            ss << std::hex << std::uppercase << std::setfill('0'); 


            for(unsigned char uc: digest){
                ss << std::setw(2) << (int)uc;
            } 

            md5string = ss.str(); 

            return md5string; 
        } 

        /*
        void hash_program_items(std::unordered_map<std::string, std::string>& hashMap){
            std::vector<std::string> vec;
            get_config_items_current(vec);

            for(const auto& entry : vec){
                const std::string& md5 = get_md5hash(entry);
                hashMap[entry] = md5;
            }
        }


        void hash_saved_items(std::unordered_map<std::string, std::string>& hashMap){
            std::vector<std::string> vec;
            get_config_items_saved(vec);

            for(const auto& entry : vec){
                const std::string& md5 = get_md5hash(entry);
                hashMap[entry] = md5;
            }
        }
        */

        int is_identical(){
            const std::string savePath = get_newest_backup_path();
            const std::string dbPath = savePath + "\\" + "ConfigSync-PathDatabase.bin";
            const std::string hbPath = savePath + "\\" + "ConfigSync-Hashbase.csv";

            database db(dbPath);
            std::map<std::string, std::string> pathMap;
            db.readStringMap(pathMap);

            hashbase h;

            if(!std::filesystem::exists(hbPath)){
                
                for(const auto& [pathA, pathB] : pathMap){
                    // TODO: Multithread the hashing
                    const std::string hashA = get_md5hash(pathA);
                    const std::string hashB = get_md5hash(pathB);

                    if(hashA != hashB){
                        return 0;
                    }
                    
                    h.ha.push_back(hashA);
                    h.hb.push_back(hashB);
                    h.pa.push_back(pathA);
                    h.pb.push_back(pathB);
                }

                std::ofstream of(hbPath);
                of.close();
                database::storeHashbase(hbPath, h);
            }
            else{
                // Hash progFiles, compare against Hashbase
                for(const auto)
            }
        }
};
#endif

                // for(const auto& [progPath, progHash] : progHashes){
                //     const std::unordered_map<std::string, std::string>::iterator& savHashesIt = savHashes.find(progPath); // Get an Iterator pointing to the corresponding path
                //     if(savHashesIt == savHashes.end() || savHashesIt->second)