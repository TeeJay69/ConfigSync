#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <programs.hpp>
#include <analyser.hpp>


int main(){
    programConfig jackett("Jackett");
    std::vector<std::string> jackettPaths = jackett.getFilePaths();

    programConfig prowlarr("Prowlarr");
    std::vector<std::string> prowlarrPaths = prowlarr.getFilePaths();

    programConfig qbittorrent("qBittorrent");
    std::vector<std::string> qbittorrentPaths = qbittorrent.getFilePaths();

    const std::filesystem::path jackettDir = "ConfigArchive\\Jackett"; // should be in class of program.hpp
    const std::filesystem::path prowlarrDir = "ConfigArchive\\Prowlarr"; // should be in class of program.hpp
    const std::filesystem::path qbittorrentDir = "ConfigArchive\\qBittorrent"; // should be in class of program.hpp

    if(std::filesystem::exists(jackettDir)){
        // something
    }
    else{
        std::filesystem::create_directories(jackettDir);
        // Make first backup
    }

    if(std::filesystem::exists(prowlarrDir)){
        // 
    }
    else{
        std::filesystem::create_directories(prowlarrDir);
    }

    if(std::filesystem::exists(qbittorrentDir)){
        // 
    }
    else{
        std::filesystem::create_directories(qbittorrentDir);
    }

    
    return 0;
}