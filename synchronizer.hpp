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
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <chrono>
#include <ranges>
#include <iomanip>
#include <openssl\md5.h>
#include <openssl\sha.h>
#include <sstream>
#include <utility>
#include "synchronizer.hpp"
#include "ANSIcolor.hpp"

struct hashbase{
    std::vector<std::pair<std::string, std::string>> hh;
    std::vector<std::pair<std::string, std::string>> pp;

    std::ranges::zip_view<std::ranges::ref_view<std::vector<std::pair<std::string, std::string>>>, std::ranges::ref_view<std::vector<std::pair<std::string, std::string>>>> zipview(){ // or auto
        return std::views::zip(hh, pp);
    }
};

struct Index {
    std::vector<std::pair<unsigned long long, std::string>> time_uuid;
};


class cfgsexcept : public std::exception {
    private:
        const char* message;

    public:
        cfgsexcept(const char* errorMessage) : message(errorMessage) {}

        const char* what() const noexcept override{
            return message;
        }
};

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

class logs{
    public:
        
        static const std::string timestamp(){
            auto now = std::chrono::system_clock::now();
            std::string formatted_time = std::format("{0:%F_%Y-%M-%S}", now);

            return formatted_time;
        }


        static void log_cleaner(const std::string& dir, const unsigned& limit){

            std::vector<std::filesystem::path> fvec;
            for(const auto& x : std::filesystem::directory_iterator(dir)){
                fvec.push_back(x);
            }

            if(fvec.size() - 1  > limit){
                
                std::sort(fvec.begin(), fvec.end());
                try{
                    std::filesystem::remove(fvec[0]);
                    log_cleaner(dir, limit); // Check again
                }
                catch(std::filesystem::filesystem_error& err){
                    throw std::runtime_error(err);
                }
            }
        }


        static std::ofstream session_log(const std::string& exeLocation, const unsigned& limit){
            
            const std::string logDir = exeLocation + "\\logs";
            const std::string logPath = logDir + "\\" + timestamp();

            if(!std::filesystem::exists(logDir)){
                std::filesystem::create_directories(logDir);
            }
            
            log_cleaner(logDir, limit);

            std::ofstream logf(logPath);
            if(!logf.is_open()){
                throw cfgsexcept("Failed to open logfile");
            }
            
            return logf;
        }


        static const std::string ms(const std::string& message){
            const std::string m = timestamp() + ":    " + message;

            return m;
        }

};
class ProgramConfig {
    private:
        std::string exeLocation;
    public:
        ProgramConfig(const std::string& executableLocation) : exeLocation(executableLocation) {} // Constructor
        
        struct ProgramInfo{
            const std::string programName;
            const std::vector<std::string> configPaths;
            const std::string archivePath;
            const std::unordered_map<std::string, std::string> pathGroups;
            const bool hasGroups;
            const std::vector<std::string> processNames;
        };
        
        enum supported {
            Jackett,
            Prowlarr,
            qBittorrent
        };

        /**
         * @brief Returns a set containing all supported programs. Includes multiple name variations. 
         * @note Should only be used for verifying user input.
         */
        static const std::unordered_set<std::string> get_support_list(){           
            std::unordered_set<std::string> list = {
                "Jackett", "jackett",
                "Prowlarr", "prowlarr",
                "qBittorrent", "qbittorrent"
            };
            
            return list;
        }

        static const std::unordered_set<std::string> get_unique_support_list(){
            std::unordered_set<std::string> list = {
                "Jackett",
                "Prowlarr",
                "qBittorrent"
            };

            return list;
        }

        static const std::string get_username(){
            const std::string x = std::getenv("username");
            return x;
        }

        const ProgramInfo get_ProgramInfo(const std::string& pName){
            const std::string userName = get_username(); 

            if(pName == "Jackett" || pName == "jackett"){
                return 
                {
                    "Jackett",
                    {"C:\\ProgramData\\Jackett"},
                    exeLocation + "\\ConfigArchive\\Jackett",
                    {},
                    false,
                    {"JackettTray.exe", "JackettConsole.exe"}
                };
            }
            
            else if(pName == "Prowlarr" || pName == "prowlarr"){
                return
                {       
                    "Prowlarr",
                    {"C:\\ProgramData\\Prowlarr"},
                    exeLocation + "\\ConfigArchive\\Prowlarr",
                    {},
                    false,
                    {"Prowlarr.Console.exe"}
                };
            }
            else if(pName == "qBittorrent" || pName == "qbittorrent"){
                const std::string logs = "C:\\Users\\" + userName + "\\AppData\\Local\\qBittorrent";
                const std::string preferences = "C:\\Users\\" + userName + "\\AppData\\Roaming\\qBittorrent";
                
                return 
                {
                    "qBittorrent",
                    {logs, preferences},
                    exeLocation + "\\ConfigArchive\\qBittorrent",
                    {{logs, "logs"}, {preferences, "preferences"}},
                    true,
                    {"qBittorrent.exe", "qbittorrent.exe"}
                };
            }
        }



};
class database {
    private:
        const std::string filePath;
    public:
        database(const std::string& path) : filePath(path) {}

        static void encodeStringWithPrefix(std::ofstream& file, std::string& str){
            // get length of string
            size_t length = str.length();

            // Serialize length prefix and String (to binary)
            file.write(reinterpret_cast<const char*>(&length), sizeof(size_t)); // Size_t is a fixed number of bytes that we use to store the length of the following string 
            file.write(str.c_str(), length); // (c_str) char* points to a memory buffer with raw binary
        }
        

        static std::string encryptString(std::string str){
            std::vector<std::string> keys = {
                "lsßg8shefayy9zuier10101938949kjsiwwUIEJAPCNrßrprl",
                "pa0ßßrYfokfjsvqpplmaxncjviibuhbvgzztfcdrrexswyqqe62293",
                "pksjavebtoETnsa2123",
                "CvWie921010LsdpcaAPdpkBVuronWOrutbaao"
            };

            std::vector<char> charVector(str.begin(), str.end());
            
            for(const auto& key : keys){
                for(size_t i = 0; i != charVector.size(); i++){

                    charVector[i] ^= key[i % key.size()];
                }
            }

            str.assign(charVector.begin(), charVector.end());
            return str;
        }


        static std::string read_lenght_prefix_encoded_string(std::ifstream& file){
            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (readStringMap)");
            }
            
            size_t prefix;
            std::string str;

            // Read string
            file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t)); // Read length prefix into "size_t prefix" from binary, number of bytes of size_t
            str.resize(prefix); // adjust string buffer for .read
            file.read(str.data(), prefix);
            
            file.close();
            
            return str;
        }


        static std::string read_length_prefix_encoded_encrypted_string(std::ifstream& file){
            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (readStringMap)");
            }
            
            size_t prefix;
            std::string str;

            // Read string
            file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t)); // Read length prefix into "size_t prefix" from binary, number of bytes of size_t
            str.resize(prefix); // adjust string buffer for .read
            file.read(str.data(), prefix);

            // Decrypt string
            str = encryptString(str);
            
            file.close();
            
            return str;
        }


        void storeStringMap(std::map<std::string, std::string>& map){
            std::ofstream file(filePath);

            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (storeStringMap)");
            }
            
            for(const auto& pair : map){
                std::string key = encryptString(pair.first);
                std::string value = encryptString(pair.second);    
                encodeStringWithPrefix(file, key);
                encodeStringWithPrefix(file, value);
            }
            file.close();
            // encrypt_decrypt_File();
        }
        

        void readStringMap(std::map<std::string, std::string>& map){
            std::ifstream file(filePath);

            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (readStringMap)");
            }
            
            size_t prefix;
            size_t vPrefix;
            std::string key1;
            std::string value1;

            // Decrypt file
            // encrypt_decrypt_File();

            while(true){

                // Read key
                file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t)); // Read length prefix into "size_t prefix" from binary, number of bytes of size_t
                key1.resize(prefix); // adjust string buffer for .read
                file.read(key1.data(), prefix);

                // Read value1
                file.read(reinterpret_cast<char*>(&vPrefix), sizeof(size_t));
                value1.resize(vPrefix);
                file.read(value1.data(), vPrefix);

                // Decrypt
                std::string key = encryptString(key1);
                std::string value = encryptString(value1);

                // Insert in map
                map[key] = value;
                
                if(file.eof()){
                    file.close();
                    return;
                }
            }
        }
        

        void storeIntMap(std::map<unsigned long long, std::string>& map){
            std::ofstream file(filePath);
            if(!file.is_open()){
                throw std::runtime_error("Error creating file. class (database), storeIntMap()");
            }

            for(const auto& pair : map){
                // encode key
                int key = pair.first;
                std::string s = std::to_string(key);
                encodeStringWithPrefix(file, s);

                // encode value
                std::string value = pair.second;
                encodeStringWithPrefix(file, value);
            }

            file.close();
        }
        

        void readIntMap(std::map<unsigned long long, std::string>& map){
            std::ifstream file(filePath);
            if(!file.is_open()){
                throw std::runtime_error("Error creating file. class (database), readIntMap()");
            }

            size_t prefix;
            size_t valuePrefix;
            std::string keyString;
            std::string value;

            while(true){
                // Read keys:
                file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t));
                keyString.resize(prefix);
                file.read(keyString.data(), prefix);

                // Read value
                file.read(reinterpret_cast<char*>(&valuePrefix), sizeof(size_t));
                value.resize(valuePrefix);
                file.read(value.data(), valuePrefix);

                // Add to map
                unsigned long long key = std::stoi(keyString);
                map[key] = value;
            
            }
            
            file.close();
        }

        /**
         * @brief Write a hashbase to file in CSV format.
         * 
         */
        static void storeHashbase(const std::string& path, const hashbase& H){
            std::ofstream hf;
            if(!std::filesystem::exists(path)){
                hf.open(path);
                if(!hf.is_open()){
                    throw cfgsexcept("Failed to open file: Error creating file. ");
                }
            }
            else{
                hf.open(path);
            }
            if(!hf.is_open()){
                throw cfgsexcept("Failed to open file: Error opening. ");
            }

            
            if(H.hh.empty() && !H.pp.empty()){
                for(const auto& pair : H.pp){
                    hf << "0" << "," << "0" << "," << pair.first << "," << pair.second << ",\n";
                }
            }
            else if(!H.hh.empty() && !H.pp.empty()){
                for(const auto& [pairA, pairB] : std::views::zip(H.hh, H.pp)){
                    hf << pairA.first << "," << pairA.second << "," << pairB.first << "," << pairB.second << ",\n";
                }
            }
            
            
            hf.close();
        }


        static void readHashbase(const std::string& path, hashbase& H, std::ofstream& logfile){
            if(!std::filesystem::exists(path)){
                throw cfgsexcept("Failed to open file: file not found. " + __LINE__);
            }

            std::ifstream hf;
            hf.open(path);
            if(!hf.is_open()){
                throw cfgsexcept("Failed to open file: Error opening. " + __LINE__);
            }
            
            std::string str;

            while(std::getline(hf, str)){
                std::istringstream iss(str);

                std::string hashA;
                std::string hashB;
                std::string pathA;
                std::string pathB;

                std::getline(iss, hashA, ',');
                std::getline(iss, hashB, ',');  
                std::getline(iss, pathA, ',');  
                std::getline(iss, pathB, ',');  

                H.hh.push_back(std::make_pair(hashA, hashB));
                H.pp.push_back(std::make_pair(pathA, pathB));
                
            }
            if(H.pp.empty()){
                logfile << logs::ms("hashbase path vector is empty after reading the hashbase.\n");
            }
            hf.close();
        }



        void readIndex(Index& IX){

            if(!std::filesystem::exists(filePath)){
                throw cfgsexcept("Failed to open file: file not found. " + __LINE__);
            }

            std::ifstream indexFile;
            indexFile.open(filePath);
            if(!indexFile.is_open()){
                throw cfgsexcept("Failed to open file: Error opening. " + __LINE__);
            }

            std::string str;

            while(std::getline(indexFile, str)){
                std::istringstream iss(str);

                
                std::string timestamp;
                std::string uuid;

                std::getline(iss, timestamp, ',');
                std::getline(iss, uuid, ',');

                IX.time_uuid.push_back(std::pair(std::stoull(timestamp), uuid));
            }
            
            indexFile.close();
        }



};
class analyzer{
    private:
        const std::vector<std::string>& programPaths;
        const std::string& programName;
        const std::string& exeLocation;
        std::ofstream& logfile;
    public:
        analyzer(const std::vector<std::string>& path, const std::string& name, const std::string& exeLocat, std::ofstream& logFile) : programPaths(path), programName(name), exeLocation(exeLocat), logfile(logFile) {}


        const std::string archivePath = exeLocation + "\\ConfigArchive\\" + programName;
        const std::string configArchive = exeLocation + "\\ConfigArchive\\";

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

        /**
         * @brief Get Index of programs prior restore backups.  
         * @param program Program 
         * @param exePath Executable location
         * @note The Index structure contains the timestamps and UUIDs for all backups of the program. The last element is the newest.
         */
        static Index get_Index(const std::string& program, const std::string& exePath){
            const std::string pBackLoc = exePath + "\\ConfigBackup\\" + program;
            if(!std::filesystem::exists(pBackLoc) || std::filesystem::is_empty(pBackLoc)){
                return {};
            }

            const std::string &IXPath = exePath + "\\ConfigBackup\\" + program + "\\Index.csv";
            Index IX;
            database dbUX(IXPath);
            dbUX.readIndex(IX);

            return IX;
        }

        
        // Check if a programs archive is empty
        int is_archive_empty(){
            if(std::filesystem::is_empty(configArchive) || std::filesystem::is_empty(archivePath)){
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
class organizer{
    public:
        
        // Limit number of saves inside the config archive.
        void limit_enforcer_configarchive(const int& maxdirs, const std::string& savepath){
            std::vector<std::filesystem::path> itemList;
            
            for(const auto& item : std::filesystem::directory_iterator(savepath)){ // Iterate over date dirs
                if(item.is_directory()){

                    // Check if directory begins with "2"
                    if(item.path().filename().generic_string()[0] == '2'){
                        
                        itemList.push_back(item.path());
                    }
                }
            }
            
            // Logic for removing the oldest save
            if(itemList.size() > maxdirs){
                std::sort(itemList.begin(), itemList.end());
                
                try{
                    std::filesystem::remove(itemList[0]); // remove oldest save
                    limit_enforcer_configarchive(maxdirs, savepath); // Self reference
                }
                catch(std::filesystem::filesystem_error& error){
                    std::cerr << ANSI_COLOR_RED << "Failed to remove directory. (Class: organizer). Error Code: 38 & <" << ">" << ANSI_COLOR_RESET << std::endl;
                    throw cfgsexcept(error.what());
                }
            }
        }


        static void index_cleaner(const int& maxdirs, Index& IX, const std::string& program_directory){
            if(IX.time_uuid.size() > maxdirs){
                try{
                    recurse_remove(std::filesystem::path(program_directory + "\\" + IX.time_uuid[0].second));
                    // Remove element from index 0 (oldest)
                    IX.time_uuid.erase(IX.time_uuid.begin());
                    // index_cleaner(maxdirs, IX, program_directory);
                }
                catch(cfgsexcept& err){
                    std::cerr << err.what() << std::endl;
                }
            }
        }
};
class synchronizer{
    private:
        const std::string programName;
        const std::string exeLocation;
        std::ofstream& logfile;
    public:
        synchronizer(const std::string& pname, const std::string& exeLocat, std::ofstream& logf) : programName(pname), exeLocation(exeLocat), logfile(logf) {}
        

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


        static unsigned long long timestamp(){
            std::chrono::time_point<std::chrono::system_clock> timePoint;
            timePoint = std::chrono::system_clock::now();
            unsigned long long t = std::chrono::system_clock::to_time_t(timePoint);
            
            return t;
        }


        static std::string timestamp_to_string(unsigned long long timestamp){
            std::time_t ts = static_cast<std::time_t>(timestamp);
            std::tm* tm = std::localtime(&ts);
            
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm);
            
            return std::string(buffer);
        
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
         * @brief Copy the program paths to the respective date directory inside the archive.
         * @param archivePathAbs This is the path to the directory in the archive, that contains all the date directories.
         * @param dateDir A specific date for the directory that the items are copied to.
         * @note Uses private members 'programPaths', 'programName' and 'exeLocation'.
         */
        int copy_config(const std::string& archivePathAbs, const std::string& dateDir, const std::vector<std::string>& programPaths){ // archivePathAbs = programs directory containing the date directories
            
            ProgramConfig PC(exeLocation);
            const auto appInfo = PC.get_ProgramInfo(programName);

            // create first date folder if it doesnt exist yet
            if(std::filesystem::is_empty(std::filesystem::path(archivePathAbs))){
                char* date = ymd_date_cstyle();

                std::filesystem::create_directory(exeLocation + "\\" + "ConfigArchive\\" + programName + "\\" + date); // Create archive dir and the subdir "dateDir"
            }
            


            const std::filesystem::path datePath = (archivePathAbs / std::filesystem::path(dateDir));
            if(!std::filesystem::exists(datePath)){ // If date dir doesnt exist, create it.

                std::filesystem::create_directories(datePath);
            }
            else if(!std::filesystem::is_empty(datePath)){ // If exists but not empty, remove dir and recreate it. Handles cases where user runs sync command multiple times on the same day.

                recurse_remove(archivePathAbs);

                std::filesystem::create_directories(datePath);
            }

            
  

            // hashbase location is inside the dateDir
            const std::string hashbasePath = archivePathAbs + "\\" + dateDir + "\\ConfigSync-Hashbase.csv";

            hashbase H; // Initialize hashbase
            std::optional<std::reference_wrapper<std::vector<std::pair<std::string,std::string>>>> ppRef = std::ref(H.pp);

            /* Copy Process */
            std::unordered_map<std::string, std::string> id;
            int groupFlag = 0;
            if(appInfo.hasGroups == true){ // Check for groups
                id = appInfo.pathGroups;
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


            database dbX(hashbasePath);

            dbX.storeHashbase(hashbasePath, H); // Write the hashbase to file

            /* Create username file */
            std::ofstream idfile(archivePathAbs + "\\" + dateDir + "\\id.bin");
            std::string username = ProgramConfig::get_username();
            dbX.encodeStringWithPrefix(idfile, username);

            
            return 1;
        }



        int revert_restore(const std::string& uuid){
            const std::string hashbasePath = exeLocation + "\\ConfigBackup\\" + programName + "\\" + uuid + "\\ConfigSync-Hashbase.csv";
            database db(hashbasePath);
            hashbase H;
            db.readHashbase(hashbasePath, H, logfile);

            for(const auto& pair : H.pp){
                try{
                    std::filesystem::permissions(pair.first, std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write);
                    std::filesystem::remove(pair.first);
                    std::filesystem::copy_file(std::filesystem::path(pair.second), pair.first, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::update_existing);
                }
                catch(cfgsexcept& error){
                    std::cerr << error.what() << std::endl;
                    return 0;
                }
            }
            
            return 1;
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
        int restore_config(const std::string dateDir, const int& recyclebinlimit, const std::vector<std::string>& programPaths){ // Main function for restoring. Uses generate_UUID, timestamp_objects, backup_config_for_restore, backup_config_for_restore, rebuild_from_backup
            // Check if programPaths exist
            for(const auto& item : programPaths){
                if(!std::filesystem::exists(std::filesystem::path(item))){
                    std::cerr << "Error: programs config path does not exist" << std::endl;
                    return 0;
                }
            }

            std::cout << ANSI_COLOR_YELLOW << "Backing up program config..." << ANSI_COLOR_RESET << std::endl;

            /* Backup: */
            /* Copy current config to ConfigBackup  */
            const std::string backupDir = exeLocation + "\\ConfigBackup\\" + programName;
            const std::string dirUUID = generate_UUID();
            std::map<unsigned long long, std::string> recycleMap; // Stores recycleBin items with timestamps

            if(!std::filesystem::exists(backupDir)){

                std::filesystem::create_directories(backupDir);

            }

            copy_config(backupDir, dirUUID, programPaths);

            
            // Load index file
            std::string indexPath = backupDir + "\\Index.csv";
            // Try to create the index file when it doesnt exist

            if(!std::filesystem::exists(backupDir + "\\Index.csv")){

                std::ofstream indexFile(indexPath);

                if(!indexFile.is_open()){
                    throw cfgsexcept("Error creating index file\n");
                    logfile << logs::ms("Error creating index file\n");
                }


                indexFile << timestamp << "," << dirUUID << ",\n";
            }

            
            // Append timestamp and UUID to index
            std::ofstream indexFile(indexPath, std::ios_base::app);
            if(!indexFile.is_open()){
                throw cfgsexcept("Error creating index file\n");
                logfile << logs::ms("Error creating index file\n");
            }
            indexFile << timestamp << "," << dirUUID << ",\n";

            indexFile.close();

            // Read index file into memory
            Index IX;
            database ss(indexPath);
            ss.readIndex(IX);

            // Enforce max dir limit
            organizer janitor;
            organizer::index_cleaner(recyclebinlimit, IX, backupDir);
            
            // Write updated index
            std::ofstream iXfile(indexPath, std::ios::trunc);
            if(iXfile.is_open()){
                for(const auto& pair : IX.time_uuid){
                    iXfile << pair.first << "," << pair.second << ",\n";
                }
            }
            
            /* Restore: */
            // Get paths from ConfigArchive
            const std::string databasePath = (exeLocation + "\\ConfigArchive\\" + programName + "\\" + std::filesystem::path(dateDir).filename().string() + "\\ConfigSync-Hashbase.csv");
            hashbase H;
            database::readHashbase(databasePath, H, logfile);

            /* Get username file */
            std::string idPath = exeLocation + "\\ConfigArchive\\" + programName + "\\" + std::filesystem::path(dateDir).filename().string() + "\\id.bin";
            std::ifstream idFile(idPath);
            std::string id = database::read_lenght_prefix_encoded_string(idFile);


            if(id != ProgramConfig::get_username()){ // Check if username is still valid

                transform_pathvector_new_username(H.pp, id); // Update username
            }



            
            // Replace config
            for(const auto& pair : H.pp){
                try{
                    std::filesystem::permissions(pair.first, std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write);
                    std::filesystem::remove(pair.first);
                    std::filesystem::copy_file(std::filesystem::path(pair.second), pair.first, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::update_existing);
                }
                catch(cfgsexcept& copyError){ // Error. Breaks for loop 
                    std::cerr << "Aborting synchronisation: ( " << copyError.what() << ")" << std::endl;
                    std::cout << "Rebuilding from backup..." << std::endl;
                    
                    // Rebuild from backup
                    if(revert_restore(dirUUID) != 1){

                        std::cerr << ANSI_COLOR_RED << "Fatal: Failed to rebuild from backup.\n Please verify that none of your selected programs components are missing or corrupted." << ANSI_COLOR_RESET << std::endl;
                        std::cerr << "You may need to reinstall the affected application." << std::endl;  
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
class Process{
    public:

        int findPID(const char *procname) {

            HANDLE hSnapshot;
            PROCESSENTRY32 pe;
            int pid = 0;
            BOOL hResult;

            // snapshot of all processes in the system
            hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

            // initializing size: needed for using Process32First
            pe.dwSize = sizeof(PROCESSENTRY32);

            // info about first process encountered in a system snapshot
            hResult = Process32First(hSnapshot, &pe);

            // retrieve information about the processes
            // and exit if unsuccessful
            while (hResult) {
                // if we find the process: return process ID
                if (strcmp(procname, pe.szExeFile) == 0) {
                pid = pe.th32ProcessID;
                break;
                }  
                hResult = Process32Next(hSnapshot, &pe);
            }

            // closes an open handle (CreateToolhelp32Snapshot)
            CloseHandle(hSnapshot);
            return pid;
        }

        /**
         * @brief Kill a process by its name
         * @param processName Process name
         * 
         */
        int killProcess(const char* processName){

            const int x = findPID(processName);
            
            if(x == NULL || x == 0){
                return 0;
            }


            const auto adpservice = OpenProcess(PROCESS_TERMINATE, false, x);
            TerminateProcess(adpservice, 1);
            CloseHandle(adpservice);
            
            return 1;
        }
};
#endif