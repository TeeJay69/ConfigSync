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

    std::string encryptString(std::string str){
        std::vector<std::string> keys = {
            "lsßg8shefayy9zuier10101938949kjsiwwUIEJAPCNrßrprl",
            "pa0ßßrYfokfjsvqpplmaxncjviibuhbvgzztfcdrrexswyqqe62293",
            "pksjavebtoETnsa2123",
            "CvWie921010LsdpcaAPdpkBVuronWOrutbaao"
        };

        std::vector<char> charVector(str.begin(), str.end());
        
        for(const auto& key : keys){
            for(size_t i = 0; i != charVector.size(); i++){

                charVector[i] ^= key[i % key.size()];
            }
        }

        str.assign(charVector.begin(), charVector.end());
        return str;
    }

    void storeStringMap(std::ofstream& file, std::map<std::string, std::string>& map){
        if(!file.is_open()){
            throw std::runtime_error("Failed to open file in, (storeStringMap)");
        }

        for(const auto& pair : map){
            std::string first = encryptString(pair.first);
            std::string value = encryptString(pair.second);
            encodeStringPrefix(file, first);
            encodeStringPrefix(file, value);
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

            // decrypt
            std::string key2 = encryptString(key);
            std::string value2 = encryptString(value);

            // Insert in map
            map[key2] = value2;
            
            if(file.eof()){
                file.close();
                return;
            }
        }
    }
};
#endif