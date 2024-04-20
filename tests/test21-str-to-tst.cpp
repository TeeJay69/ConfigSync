#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip> // Required for std::get_time

template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
static const T str_to_timestamp(const std::string& datetime) {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M");

    if (ss.fail()) {
        ss.clear(); // Clear the error state
        ss.str(datetime); // Reset the stringstream with the original input
        // Try parsing without hour and minute
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (ss.fail()) {
            return 0;
        }
    }
    std::time_t timestamp = std::mktime(&tm);
    return static_cast<T>(timestamp);
}

int main(){
    const std::string str = "2024-04-20 23:03";
    const std::string str1 = "2024-04-20";
    unsigned long long u = str_to_timestamp<unsigned long long>(str);
    unsigned long long u1 = str_to_timestamp<unsigned long long>(str1);
    std::cout << "string: " << str << "\n" << "timestamp: " << u << std::endl;
    std::cout << "string: " << str1 << "\n" << "timestamp: " << u1 << std::endl;
}