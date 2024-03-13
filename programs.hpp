#ifndef PROGRAMS_HPP
#define PROGRAMS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <unordered_set>
#include <map>
#include "CFGSExcept.hpp"

class programconfig{
    private:
        std::string programName;
        std::string exeLocation;
    public:
        programconfig(const std::string& name, const std::string& executableLocation) : programName(name), exeLocation(executableLocation) {} // Constructor
        
        
        // Returns a vector containing supported programs.
        static const std::unordered_set<std::string> get_support_list(){           
            std::unordered_set<std::string> list = {
                "Jackett",
                "Prowlarr",
                "qBittorrent"
            };
            
            return list;
        }


        static int has_groups(const std::string& program){
            std::unordered_set<std::string> grouped = {
                "qBittorrent"
            };

            if(grouped.contains(program)){
                return 1;
            }
            
            return 0;
        }


        static const std::string get_username(){
            const std::string x = std::getenv("username");
            return x;
        }


        std::vector<std::string> get_config_paths(){
            
            const std::string userName = get_username();

            // Jackett
            if(programName == "Jackett"){
                std::vector<std::string> jackettPaths = {"C:\\ProgramData\\Jackett"};
                return jackettPaths;
            }
            // Prowlarr
            else if(programName == "Prowlarr"){
                std::vector<std::string> prowlarrPaths = {"C:\\ProgramData\\Prowlarr"};
                return prowlarrPaths;
            }
            // qBittorrent
            else if(programName == "qBittorrent"){
                const std::string logs = "C:\\Users\\" + userName + "\\AppData\\Local\\qBittorrent";
                const std::string preferences = "C:\\Users\\" + userName + "\\AppData\\Roaming\\qBittorrent";

                std::vector<std::string> qBittorrentPaths = {logs, preferences};
                return qBittorrentPaths;
            }
            else{
                const std::string nameError = "Error: Program \"" + programName + "\" is currently unsupported";
                throw std::invalid_argument(nameError);
            }
        }


        const std::filesystem::path get_archive_path(){

            if(programName == "Jackett"){
                const std::string jackettArchivePath = exeLocation + "\\ConfigArchive\\Jackett";
                return jackettArchivePath;
            }

            else if(programName == "Prowlarr"){
                const std::string prowlarrArchivePath = exeLocation + "\\ConfigArchive\\Prowlarr";
                return prowlarrArchivePath;
            }

            else if(programName == "qBittorrent"){
                const std::string qBittorrentArchivePath = exeLocation + "\\ConfigArchive\\qBittorrent";
                return qBittorrentArchivePath;
            }
            else{
                const std::string nameError = "Error: Program \"" + programName + "\" is currently unsupported";
                throw std::invalid_argument(nameError);
            }
            
            // ...

        }

        
        /**
         * @brief Returns a map of strings with path as key and name as value.
         * @note Each entry is a unique name for each config_path of the program.
         */
        std::unordered_map<std::string, std::string> get_path_groups(){

            const std::string userName = get_username();
            std::string logs = "C:\\Users\\" + userName + "\\AppData\\Local\\qBittorrent";
            std::string preferences = "C:\\Users\\" + userName + "\\AppData\\Roaming\\qBittorrent";
            
            if(programName == "qBittorrent"){
                return {{logs, "logs"}, {preferences, "preferences"}};
            }


            std::unordered_map<std::string, std::string> x;
            return x;
        }

};

#endif