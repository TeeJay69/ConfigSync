#ifndef ORGANIZER_HPP
#define ORGANIZER_HPP

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "ANSIcolor.hpp"


class organizer{
    private:
        std::string dateDirParent;
    public:
        organizer(const std::string& dateDirParentPath) : dateDirParent(dateDirParentPath) {}


        void limit_enforcer(const int& maxdirs){
            // count the number of saved configs
            std::vector<std::filesystem::path> itemList;
            
            for(const auto& item : std::filesystem::directory_iterator(dateDirParent)){
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
};


#endif