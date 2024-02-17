#ifndef PROGRAMS_HPP
#define PROGRAMS_HPP

#include <iostream>
#include <string>
#include <vector>

class programConfig{
    private:
        std::string programName;
    public:
        programConfig(const std::string& name) : programName(name) {} // Constructor
        
        std::string getUsername(){
            std::string x = std::getenv("username");
            return x;
        }

        std::vector<std::string> getFilePaths(){
            std::string userName = getUsername();

            if(programName == "Jackett"){
                std::vector<std::string> jackettPaths = {"C:\\ProgramData\\Jackett"};
                return jackettPaths;
            }
            else if(programName == "Prowlarr"){
                std::vector<std::string> prowlarrPaths = {"C:\\ProgramData\\Prowlarr"};
                return prowlarrPaths;
            }
            else if(programName == "qBittorrent"){
                std::string preferences = " C:\\Users\\" + userName + "\\AppData\\Roaming\\qBittorrent";
                std::string logs = " C:\\Users\\" + userName + "\\AppData\\Local\\qBittorrent";

                std::vector<std::string> qBittorrentPaths = {preferences, logs};
                return qBittorrentPaths;
            }
            else{
                std::string nameError = "Error: Program \"" + programName + "\" is currently unsupported";
                throw std::runtime_error(nameError);
            }
        }
};

#endif