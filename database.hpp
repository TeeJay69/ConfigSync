#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>

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


        struct hashbase{
            std::vector<std::string> ha;
            std::vector<std::string> hb;
            std::vector<std::string> pa;
            std::vector<std::string> pb;
        };


        static void storeHashbase(const std::string& path, const hashbase& hb){
            hashbase h;
            if(!std::filesystem::exists(path)){
                throw cfgsexcept((const char*)("Failed to open file: file not found. " + __LINE__));
            }
            std::ofstream hf;
            hf.open(path);

            for(unsigned i = 0; i < h.ha.size(); i++){
                hf << h.ha[i] << "," << h.hb[i] << "," << h.pa[i] << "," << h.pb[i] << "\n";
            }
        }
};

#endif