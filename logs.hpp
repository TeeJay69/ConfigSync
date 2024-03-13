#ifndef LOGS_HPP
#define LOGS_HPP

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <set>
#include <algorithm>
#include "synchronizer.hpp"
#include "CFGSExcept.hpp"

class logs{
    public:
        
        static const std::string timestamp(){
            auto now = std::chrono::system_clock::now();
            std::string formatted_time = std::format("{0:%F_%Y-%M-%S}", now);

            return formatted_time;
        }


        static void log_cleaner(const std::string& dir, const unsigned& limit){

            std::vector<std::filesystem::path> fvec;
            for(const auto& x : std::filesystem::directory_iterator(dir)){
                fvec.push_back(x);
            }

            if(fvec.size() - 1  > limit){
                
                std::sort(fvec.begin(), fvec.end());
                try{
                    std::filesystem::remove(fvec[0]);
                    log_cleaner(dir, limit); // Check again
                }
                catch(std::filesystem::filesystem_error& err){
                    throw std::runtime_error(err);
                }
            }
        }


        static std::ofstream session_log(const std::string& exeLocation, const unsigned& limit){
            
            const std::string logDir = exeLocation + "\\logs";
            const std::string logPath = logDir + "\\" + timestamp();

            if(!std::filesystem::exists(logDir)){
                std::filesystem::create_directories(logDir);
            }
            
            log_cleaner(logDir, limit);

            std::ofstream logf(logPath);
            if(!logf.is_open()){
                throw cfgsexcept("Failed to open logfile");
            }
            
            return logf;
        }


        static const std::string ms(const std::string& message){
            const std::string m = timestamp() + ":    " + message;

            return m;
        }

};

#endif