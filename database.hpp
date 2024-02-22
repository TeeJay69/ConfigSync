#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <map>

class database{
    public:
    
    void encodeStringPrefix(std::ofstream& file, const std::string& str){
        // get length of string
        size_t length = str.length();

        // Serialize length prefix and String (to binary)
        file.write(reinterpret_cast<const char*>(&length), sizeof(size_t)); // Size_t is a fixed number of bytes that we use to store the length of the following string 
        file.write(str.c_str(), length); // (c_str) char* points to a memory buffer with raw binary
    }

    void storeStringMap(std::ofstream& file, std::map<std::string, std::string>& map){
        if(!file.is_open()){
            throw std::runtime_error("Failed to open file in, (storeStringMap)");
        }

        for(const auto& pair : map){
            encodeStringPrefix(file, pair.first);
            encodeStringPrefix(file, pair.second);
        }
        file.close();
    }

    void readStringMap(std::ifstream& file, std::map<std::string, std::string>& map){
        if(!file.is_open()){
            throw std::runtime_error("Failed to open file in, (readStringMap)");
        }
        
        size_t prefix;
        size_t vPrefix;
        std::string key;
        std::string value;

        while(true){

            // Read key
            file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t)); // Read length prefix into "size_t prefix" from binary, number of bytes of size_t
            
            key.resize(prefix); // adjust string buffer for .read
            file.read(key.data(), prefix);

            // Read value
            file.read(reinterpret_cast<char*>(&vPrefix), sizeof(size_t));
            value.resize(vPrefix);
            file.read(value.data(), vPrefix);

            // Insert in map
            map[key] = value;
            
            if(file.eof()){
                file.close();
                return;
            }
        }
    }
};
#endif