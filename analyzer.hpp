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
#include <openssl\sha.h>
#include <map>
#include <sstream>
#include <utility>
#include <ranges>
#include "database.hpp"
#include "programs.hpp"
#include "organizer.hpp"
#include "ANSIcolor.hpp"
#include "CFGSExcept.hpp"
#include "hashbase.hpp"
#include "logs.hpp"


class analyzer{
    private:
        const std::vector<std::string>& programPaths;
        const std::string& programName;
        const std::string& exeLocation;
        std::ofstream& logfile;
    public:
        analyzer(const std::vector<std::string>& path, const std::string& name, const std::string& exeLocat, std::ofstream& logFile) : programPaths(path), programName(name), exeLocation(exeLocat), logfile(logFile) {}


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

        static int has_backup(const std::string& program, const std::string& exePath){
            const std::string pBackLoc = exePath + "\\ConfigBackup\\" + program;
            if(!std::filesystem::exists(pBackLoc) || std::filesystem::is_empty(pBackLoc)){
                return 0;
            }
            
            return 1;
        }

        static Index get_Index(const std::string& program, const std::string& exePath){
            const std::string pBackLoc = exePath + "\\ConfigBackup\\" + program;
            if(!std::filesystem::exists(pBackLoc) || std::filesystem::is_empty(pBackLoc)){
                throw cfgsexcept("Error: Last backup not found. (get_last_backup)");
            }

            const std::string &IXPath = exePath + "\\ConfigBackup\\" + program + "\\Index.csv";
            Index IX;
            database db(IXPath);
            db.readIndex(IX);

            return IX;
        }

        
        // Check if a programs archive is empty
        int is_archive_empty(){
            if(std::filesystem::is_empty(archivePath)){
                return 1;
            }
            
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


        static const int has_hashbase(const std::string& path){
            const std::string hPath = path + "\\ConfigSync-Hashbase.csv";

            if(!std::filesystem::exists(hPath)){ // No hashmap found
                return 0;
            }

            return 1;
        }


        std::string get_sha256hash(const std::string& fname){
            FILE *file;

            unsigned char buf[8192];
            unsigned char output[SHA256_DIGEST_LENGTH];
            size_t len;

            SHA256_CTX sha256;

            file = fopen(fname.c_str(), "rb");

            if(file == NULL){
                const std::string err = "Error: Failed to open file to hash.\n";
                logfile << logs::ms(err);
                throw cfgsexcept(err.c_str());
            }
            else{

                SHA256_Init(&sha256);
                while((len = fread(buf, 1, sizeof(buf), file)) != 0){
                    SHA256_Update(&sha256, buf, len);
                } 
                fclose(file);
                SHA256_Final(output, &sha256);

                std::stringstream ss;
                for (int i = 0; i < SHA256_DIGEST_LENGTH; i++){
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(output[i]);
                }

                return ss.str();
            }
        }

        int is_identical(){
            const std::string savePath = get_newest_backup_path();
            const std::string hbPath = savePath + "\\" + "ConfigSync-Hashbase.csv";

            /* Hash progFiles and savefiles using paths from hashbase and compare */
            hashbase H;            
            database::readHashbase(hbPath, H, logfile);
            for(const auto& [hash, path] : std::views::zip(H.hh, H.pp)){
                
                if(!std::filesystem::exists(path.first)){
                    std::cerr << "Warning: program path not found!" << std::endl;
                    logfile << logs::ms("Warning:") <<  " program file [" << path.first << "] not found!" << std::endl;
                    return 0;
                }
                
                else if(!std::filesystem::exists(path.second)){
                    logfile << logs::ms("Warning:") << " archived file [" << path.second << "] not found!" << std::endl;
                    return 0;
                }

                const std::string &firstHash = get_sha256hash(path.first);
                const std::string &secondHash = get_sha256hash(path.second);

                if(firstHash != secondHash){
                    logfile << logs::ms("Hashes don't match.\n");
                    return 0;
                }
                else{
                    hash = std::pair(firstHash, secondHash);
                }
            }

            logfile << logs::ms("Config is identical, storing hashbase.\n");
            database::storeHashbase(hbPath, H);

            return 1;
        }
};
#endif

                // for(const auto& [progPath, progHash] : progHashes){
                //     const std::unordered_map<std::string, std::string>::iterator& savHashesIt = savHashes.find(progPath); // Get an Iterator pointing to the corresponding path
                //     if(savHashesIt == savHashes.end() || savHashesIt->second)