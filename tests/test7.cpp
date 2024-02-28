#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

std::string ymd_date() {
    const std::chrono::time_point now(std::chrono::system_clock::now());
    const std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(now));
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year()) << '-'
       << std::setw(2) << static_cast<unsigned>(ymd.month()) << '-'
       << std::setw(2) << static_cast<unsigned>(ymd.day());

    return ss.str();
}

int main() {
    std::cout << ymd_date() << std::endl;
    return 0;
}