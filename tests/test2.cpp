#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cstdlib>


void writeStringWithLengthPrefix(std::ofstream& file, const std::string& str) {
    // Write the length of the string as a prefix
    size_t length = str.length(); // Corrected to calculate the length of the string
    file.write(reinterpret_cast<const char*>(&length), sizeof(size_t));
    // Write the string data
    file.write(str.c_str(), length);    
}

void storeStringMap(const std::string& filename, const std::map<std::string, std::string>& map) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing." << std::endl;
        return;
    }
    for (const auto& pair : map) {
        writeStringWithLengthPrefix(file, pair.first);
        writeStringWithLengthPrefix(file, pair.second);
    }
    file.close();
}

// std::string readStringWithLengthPrefix(std::ifstream& file) {
//     // Read the length prefix of the string
//     size_t length;
//     if (!file.read(reinterpret_cast<char*>(&length), sizeof(size_t))) {
//         // Failed to read length prefix
//         return ""; // Return an empty string or handle the error accordingly
//     }
//     // Read the string data
//     char* buffer = (char *)malloc(length); // Cast the result of malloc to char*

//     file.read(buffer, length);
//     std::string str;
//     str.assign(buffer, length);
//     free(buffer);
//     return str;
// }
std::string readStringWithLengthPrefix(std::ifstream& file) {
    // Read the length prefix of the string
    size_t length;
    if (!file.read(reinterpret_cast<char*>(&length), sizeof(size_t))) {
        // Failed to read length prefix
        return ""; // Return an empty string or handle the error accordingly
    }
    // Read the string data
    std::string str;
    str.resize(length);
    file.read(str.data(), length);
    return str;
}

std::map<std::string, std::string> readStringMap(const std::string& filename) {
    std::map<std::string, std::string> map;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file for reading." << std::endl;
        return map;
    }
    while (true) {
        std::string key = readStringWithLengthPrefix(file);
        if (file.eof()) {
            break;  // Exit the loop if end of file is reached
        }
        std::string value = readStringWithLengthPrefix(file);
        map[key] = value;
    }
    file.close();
    return map;
}

int main() {
    // Define a sample map
    std::map<std::string, std::string> myMap;
    myMap["key1 \n LineBrea,k m,  Test"] = "value1";
    myMap["key2"] = "value2";
    myMap["key3"] = "value3";

    // Store the map to a binary file
    storeStringMap("string_map.bin", myMap);

    // Read the map from the binary file
    std::map<std::string, std::string> readMap = readStringMap("string_map.bin");

    // Print the read map
    std::cout << "Read Map:" << std::endl;
    for (const auto& pair : readMap) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    return 0;
}
