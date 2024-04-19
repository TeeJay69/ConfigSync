#include <iostream>
#include <chrono>
#include <cstdint>

static const uint64_t timestamp() {
    // Get the current time point
    std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
    
    // Convert to time_t (seconds since the Unix epoch)
    const uint64_t t = std::chrono::system_clock::to_time_t(timePoint);
    
    // Convert to days since the epoch, then back to seconds at the start of the day
    uint64_t dayTimestamp = (t / 86400) * 86400;
    
    return dayTimestamp;
}

int main(){
    std::cout << "day timestamp:" << timestamp() << std::endl;

    return 0;
}