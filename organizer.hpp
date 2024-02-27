#ifndef ORGANIZER_HPP
#define ORGANIZER_HPP

#include <iostream> 
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <map>
#include "ANSIcolor.hpp"


class organizer{
    public:

        void limit_enforcer_configarchive(const int& maxdirs, std::string& savepath){
            // count the number of saved configs
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
                }
                catch(std::filesystem::filesystem_error& error){
                    std::cerr << ANSI_COLOR_RED << "Failed to remove directory. (Class: organizer). Error Code: 38 & <" << error.what() << ">" << ANSI_COLOR_RESET << std::endl;
                }
            }
        }


        void recyclebin_cleaner(const int& maxdirs, std::map<unsigned long long, std::string>& map){
            if(map.size() > maxdirs){

                for(auto pair = map.begin(); pair != map.end();){ // Iterator, not range based.

                    std::filesystem::remove(pair->second); // Delete
                    map.erase(pair->first); // Remove element from map
                    
                    if(map.size() <= maxdirs){ // Check new size
                        break;
                    }
                    else{
                        pair++;
                    }
                }
            }
        }
};
#endif