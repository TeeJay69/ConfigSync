#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <type_traits>
#include <stdexcept>

// Function definitions
static const uint64_t timestamp() {
    std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
    const uint64_t t = std::chrono::system_clock::to_time_t(timePoint);
    return t;
}

static std::string ymd_date() {
    const std::chrono::time_point now(std::chrono::system_clock::now());
    const std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(now));

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year()) << '-'
       << std::setw(2) << static_cast<unsigned>(ymd.month()) << '-'
       << std::setw(2) << static_cast<unsigned>(ymd.day());

    return ss.str();
}

template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
static const std::string timestamp_to_str_ymd(const T& timestamp) {
    static_assert(std::is_integral<T>::value, "Timestamp must be an integral type.");
    std::time_t ts = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::gmtime(&ts);  // Use UTC time

    if (tm == nullptr) {
        throw std::runtime_error("Failed to convert timestamp to UTC time.");
    }

    char buffer[11]; // Buffer size for "YYYY-MM-DD\0"
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
    return std::string(buffer);
}

int main() {
    // Step 1: Generate a timestamp
    uint64_t ts = timestamp();
    std::cout << "Timestamp: " << ts << std::endl;

    // Step 2: Get the current date string
    std::string current_date = ymd_date();
    std::cout << "Current Date (ymd_date): " << current_date << std::endl;

    // Step 3: Convert timestamp to date string
    std::string ts_date = timestamp_to_str_ymd(ts);
    std::cout << "Date from Timestamp (timestamp_to_str_ymd): " << ts_date << std::endl;

    // Step 4: Verify that the dates match
    if (current_date == ts_date) {
        std::cout << "Success: The dates match!" << std::endl;
    } else {
        std::cout << "Error: The dates do not match." << std::endl;
    }

    return 0;
}
