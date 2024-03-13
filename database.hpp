#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <filesystem>
#include "CFGSExcept.hpp"
#include "hashbase.hpp"
#include "logs.hpp"

class database{
    private:
        const std::string filePath;
    public:
        database(const std::string& path) : filePath(path) {}

        static void encodeStringWithPrefix(std::ofstream& file, std::string& str){
            // get length of string
            size_t length = str.length();

            // Serialize length prefix and String (to binary)
            file.write(reinterpret_cast<const char*>(&length), sizeof(size_t)); // Size_t is a fixed number of bytes that we use to store the length of the following string 
            file.write(str.c_str(), length); // (c_str) char* points to a memory buffer with raw binary
        }
        

        static std::string encryptString(std::string str){
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


        static std::string read_lenght_prefix_encoded_string(std::ifstream& file){
            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (readStringMap)");
            }
            
            size_t prefix;
            std::string str;

            // Read string
            file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t)); // Read length prefix into "size_t prefix" from binary, number of bytes of size_t
            str.resize(prefix); // adjust string buffer for .read
            file.read(str.data(), prefix);
            
            file.close();
            
            return str;
        }


        static std::string read_length_prefix_encoded_encrypted_string(std::ifstream& file){
            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (readStringMap)");
            }
            
            size_t prefix;
            std::string str;

            // Read string
            file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t)); // Read length prefix into "size_t prefix" from binary, number of bytes of size_t
            str.resize(prefix); // adjust string buffer for .read
            file.read(str.data(), prefix);

            // Decrypt string
            str = encryptString(str);
            
            file.close();
            
            return str;
        }


        void storeStringMap(std::map<std::string, std::string>& map){
            std::ofstream file(filePath);

            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (storeStringMap)");
            }
            
            for(const auto& pair : map){
                std::string key = encryptString(pair.first);
                std::string value = encryptString(pair.second);    
                encodeStringWithPrefix(file, key);
                encodeStringWithPrefix(file, value);
            }
            file.close();
            // encrypt_decrypt_File();
        }
        

        void readStringMap(std::map<std::string, std::string>& map){
            std::ifstream file(filePath);

            if(!file.is_open()){
                throw std::runtime_error("Failed to open file in, (readStringMap)");
            }
            
            size_t prefix;
            size_t vPrefix;
            std::string key1;
            std::string value1;

            // Decrypt file
            // encrypt_decrypt_File();

            while(true){

                // Read key
                file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t)); // Read length prefix into "size_t prefix" from binary, number of bytes of size_t
                key1.resize(prefix); // adjust string buffer for .read
                file.read(key1.data(), prefix);

                // Read value1
                file.read(reinterpret_cast<char*>(&vPrefix), sizeof(size_t));
                value1.resize(vPrefix);
                file.read(value1.data(), vPrefix);

                // Decrypt
                std::string key = encryptString(key1);
                std::string value = encryptString(value1);

                // Insert in map
                map[key] = value;
                
                if(file.eof()){
                    file.close();
                    return;
                }
            }
        }
        

        void storeIntMap(std::map<unsigned long long, std::string>& map){
            std::ofstream file(filePath);
            if(!file.is_open()){
                throw std::runtime_error("Error creating file. class (database), storeIntMap()");
            }

            for(const auto& pair : map){
                // encode key
                int key = pair.first;
                std::string s = std::to_string(key);
                encodeStringWithPrefix(file, s);

                // encode value
                std::string value = pair.second;
                encodeStringWithPrefix(file, value);
            }

            file.close();
        }
        

        void readIntMap(std::map<unsigned long long, std::string>& map){
            std::ifstream file(filePath);
            if(!file.is_open()){
                throw std::runtime_error("Error creating file. class (database), readIntMap()");
            }

            size_t prefix;
            size_t valuePrefix;
            std::string keyString;
            std::string value;

            while(true){
                // Read keys:
                file.read(reinterpret_cast<char*>(&prefix), sizeof(size_t));
                keyString.resize(prefix);
                file.read(keyString.data(), prefix);

                // Read value
                file.read(reinterpret_cast<char*>(&valuePrefix), sizeof(size_t));
                value.resize(valuePrefix);
                file.read(value.data(), valuePrefix);

                // Add to map
                unsigned long long key = std::stoi(keyString);
                map[key] = value;
            
            }
            
            file.close();
        }

        /**
         * @brief Write a hashbase to file in CSV format.
         * 
         */
        static void storeHashbase(const std::string& path, const hashbase& H){
            std::ofstream hf;
            if(!std::filesystem::exists(path)){
                hf.open(path);
                if(!hf.is_open()){
                    throw cfgsexcept("Failed to open file: Error creating file. " + __LINE__);
                }
            }
            else{
                hf.open(path);
            }
            if(!hf.is_open()){
                throw cfgsexcept("Failed to open file: Error opening. " + __LINE__);
            }

            
            if(H.hh.empty() && !H.pp.empty()){
                for(const auto& pair : H.pp){
                    hf << "0" << "," << "0" << "," << pair.first << "," << pair.second << ",\n";
                }
            }
            else if(!H.hh.empty() && !H.pp.empty()){
                for(const auto& [pairA, pairB] : std::views::zip(H.hh, H.pp)){
                    hf << pairA.first << "," << pairA.second << "," << pairB.first << "," << pairB.second << ",\n";
                }
            }
            else{
                
            }
            
            
            hf.close();
        }


        static void readHashbase(const std::string& path, hashbase& H, std::ofstream& logfile){
            if(!std::filesystem::exists(path)){
                throw cfgsexcept("Failed to open file: file not found. " + __LINE__);
            }

            std::ifstream hf;
            hf.open(path);
            if(!hf.is_open()){
                throw cfgsexcept("Failed to open file: Error opening. " + __LINE__);
            }
            
            std::string str;

            while(std::getline(hf, str)){
                std::istringstream iss(str);

                std::string hashA;
                std::string hashB;
                std::string pathA;
                std::string pathB;

                std::getline(iss, hashA, ',');
                std::getline(iss, hashB, ',');  
                std::getline(iss, pathA, ',');  
                std::getline(iss, pathB, ',');  

                H.hh.push_back(std::make_pair(hashA, hashB));
                H.pp.push_back(std::make_pair(pathA, pathB));
                
            }
            if(H.pp.empty()){
                logfile << logs::ms("hashbase path vector is empty after reading the hashbase.\n");
            }
            hf.close();
        }

};

#endif