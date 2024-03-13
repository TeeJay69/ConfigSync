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
        };
        
        enum supported {
            Jackett,
            Prowlarr,
            qBittorrent
        };

        // Returns a set containing supported programs.
        static const std::unordered_set<std::string> get_support_list(){           
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

            if(pName == "Jackett"){
                return 
                {
                    "Jackett",
                    {"C:\\ProgramData\\Jackett"},
                    exeLocation + "\\ConfigArchive\\Jackett",
                    {},
                    false
                };
            }
            
            else if(pName == "Prowlarr"){
                return
                {       
                    "Prowlarr",
                    {"C:\\ProgramData\\Prowlarr"},
                    exeLocation + "\\ConfigArchive\\Prowlarr",
                    {},
                    false
                };
            }
            
            else if(pName == "qBittorrent"){
                const std::string logs = "C:\\Users\\" + userName + "\\AppData\\Local\\qBittorrent";
                const std::string preferences = "C:\\Users\\" + userName + "\\AppData\\Roaming\\qBittorrent";
                
                return 
                {
                    "qBittorrent",
                    {logs, preferences},
                    exeLocation + "\\ConfigArchive\\qBittorrent",
                    {{logs, "logs"}, {preferences, "preferences"}},
                    true
                };
            }
        }



};

#endif