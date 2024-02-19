#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include "analyzer.hpp"
#include "programs.hpp"
#include "synchronizer.hpp"
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>

int main(){
    
    programconfig jackett("Jackett");
    std::vector<std::string> jackettPaths = jackett.getFilePaths();

    programconfig prowlarr("Prowlarr");
    std::vector<std::string> prowlarrPaths = prowlarr.getFilePaths();

    programconfig qbittorrent("qBittorrent");
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