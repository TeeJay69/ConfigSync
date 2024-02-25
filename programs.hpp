#ifndef PROGRAMS_HPP
#define PROGRAMS_HPP

#include <iostream>
#include <string>
#include <vector>


class programconfig{
    private:
        std::string programName;
        std::string exeLocation;
    public:
        programconfig(const std::string& name, const std::string& executableLocation) : programName(name), exeLocation(executableLocation) {} // Constructor
        
        std::string getUsername(){
            std::string x = std::getenv("username");
            return x;
        }

        std::vector<std::string> getFilePaths(){
            
            std::string userName = getUsername();

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


        const std::filesystem::path get_archive_path(){

            // Jackett
            if(programName == "Jackett"){
                const std::string jackettArchivePath = exeLocation + "\\ConfigArchive\\Jackett";
                return jackettArchivePath;
            }

            // Prowlarr
            else if(programName == "Prowlarr"){
                const std::string prowlarrArchivePath = exeLocation + "\\ConfigArchive\\Prowlarr";
                return prowlarrArchivePath;
            }

            // qBittorrent
            else if(programName == "qBittorrent"){
                const std::string qBittorrentArchivePath = exeLocation + "\\ConfigArchive\\qBittorrent";
                return qBittorrentArchivePath;
            }

            // ...

        }
};

#endif