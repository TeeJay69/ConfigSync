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
#include <boost\filesystem\operations.hpp>
#include <boost\filesystem\path.hpp>


int main(int argc, char* argv[]){
    
    // Get location of exe
    boost::filesystem::path exePath(boost::filesystem::initial_path<boost::filesystem::path>());
    exePath = (boost::filesystem::system_complete(boost::filesystem::path(argv[0])));
    
    // Get programconfig
    programconfig jackett("Jackett", exePath.generic_string());
    std::vector<std::string> jackettPaths = jackett.getFilePaths();

    programconfig prowlarr("Prowlarr", exePath.generic_string());
    std::vector<std::string> prowlarrPaths = prowlarr.getFilePaths();

    programconfig qbittorrent("qBittorrent", exePath.generic_string());
    std::vector<std::string> qbittorrentPaths = qbittorrent.getFilePaths();
    
    
    if(std::filesystem::exists(jackett.get_archive_path())){
        // something
    }
    else{
        std::filesystem::create_directories(jackett.get_archive_path());
        // Make first backup
    }

    if(std::filesystem::exists(prowlarr.get_archive_path())){
        // 
    }
    else{
        std::filesystem::create_directories(prowlarr.get_archive_path());
    }

    if(std::filesystem::exists(qbittorrent.get_archive_path())){
        // 
    }
    else{
        std::filesystem::create_directories(qbittorrent.get_archive_path());
    }

    
    return 0;
}