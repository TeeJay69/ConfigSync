#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>

int main(){
    std::chrono::time_point<std::chrono::system_clock> dateTime;
    dateTime = std::chrono::system_clock::now();
    
    std::cout << dateTime << " UCT\n" << std::endl;
    
    unsigned int d = std::chrono::system_clock::to_time_t(dateTime);
    std::cout << d << '\n';
    
    auto duration = dateTime.time_since_epoch();
    unsigned long long f = duration.count();
    std::cout << f << '\n';
    
    // std::filesystem::path p("te st.bin");
    // std::ofstream(p).put('G');

    // std::filesystem::file_time_type ftime = std::filesystem::last_write_time(p);
    // std::cout << std::format("Last write time: {}", ftime) << std::endl;

    
    return 0;
}