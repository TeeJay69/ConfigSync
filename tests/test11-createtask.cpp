#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <map>
#include <chrono>
#include <thread>
#include <set>

class task{
    public:
        
        /**
         * Creates a task in the windows task scheduler.
         * @param target The file path to the target of the task.
         * @param taskfrequency The intervall and frequency of the scheduled task.
         * 
         */
        static const int createtask(const std::string& target, const std::string& taskfrequency){
            
            std::stringstream ss(taskfrequency);
            std::string token;

            const std::string timeframe;
            const std::string intervall;
            std::vector<std::string> list;

            while(getline(ss, token, ',')){
                list.push_back(token);
            }

            const std::string com = "schtasks /create /tn \"ConfigSyncTask\" /tr \"\\\"" + target + "\\\"sync\" /sc " + list[0] + " /mo " + list[1]; 
            std::cout << com << std::endl;
            
            std::system(com.c_str());
            return 1;
        }
};


int main(){
    std::string path = "c:\\data\\tj\\software\\coding\\ansicolor\\ansicolor.exe";
    std::string f = "hourly,6";

    task::createtask(path, f);


    return 0;
}