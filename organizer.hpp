#ifndef ORGANIZER_HPP
#define ORGANIZER_HPP

#include <iostream> 
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <map>
#include "ANSIcolor.hpp"
#include "database.hpp"


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
                    std::cerr << ANSI_COLOR_RED << "Failed to remove directory. (Class: organizer). Error Code: 38 & <" << error.what() << ">" << ANSI_COLOR_RESET << std::endl;
                }
            }
        }


        // Limit number of items in the recyclebin of the temp directory for configBackups.
        // All dirs inside RecycleBin are accounted for in the RecycleBinMap file.
        // Checks for the limit and removes when necessary both the directory from the filesystem and the map.
        void recyclebin_cleaner(const int& maxdirs, std::map<unsigned long long, std::string>& map, const std::string& mapPath){
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

                database db(mapPath);
                db.storeIntMap(map);
            }
        }


        static void index_cleaner(const int& maxdirs, Index& IX, const std::string& program_directory){
            if(IX.time_uuid.size() > maxdirs){
                try{
                    synchronizer::recurse_remove(program_directory + "\\" + IX.time_uuid[0].second);
                    // Remove element from index 0 (oldest)
                    IX.time_uuid.erase(IX.time_uuid.begin());
                    index_cleaner(maxdirs, IX, program_directory);
                }
                catch(cfgsexcept& err){
                    std::cerr << err.what() << std::endl;
                }
            }
        }
};
#endif