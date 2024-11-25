#ifndef CONFIGSYNC_HPP
#define CONFIGSYNC_HPP

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>
#include <boost\uuid\uuid_io.hpp>
#include <boost\regex.hpp>
#include <boost\filesystem.hpp>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <chrono>
#include <ranges>
#include <iomanip>
#include <openssl\md5.h>
#include <openssl\sha.h>
#include <sstream>
#include <utility>
#include "ANSIcolor.hpp"
#include <fstream>
#include <windows.h>
#include <type_traits>
#include <locale>
#include <codecvt>
#include <cstdint>
#include <comdef.h>

extern int verbose;
extern const std::string uName;
extern const int log_limit_extern;
extern std::string pLocat;
extern const std::string sessionStamp;
extern std::string archiveDir;

namespace CS {
    class Args {
        public:
            /**
             * @brief Checks for presence of specific strings in the argument array passed to the program
             * @param argv Argument array
             * @param argc Argument count
             * @param cmp const char* to compare
             * @return 1 if string is found, 0 if not
             */
            static int argcmp(char** argv, int argc, const char* cmp){
                for(int i = 0; i < argc; i++){
                    if(strcmp(argv[i], cmp) == 0){
                        return 1;
                    }
                }
                return 0;
            }

            /**
             * @brief Checks for presence of specific strings in the argument array passed to the program and stores argument after match in buffer.
             * @param buff std::string to store the next argument after a match (argv[match+1]).
             * @param argv Argument array.
             * @param argc Argument count.
             * @param cmp const char* to compare.
             * @return 1 on match. 0 on nomatch.
             * @note if matched argument is last one, it will return 0, as no value can be assigned. argfcmp stands for argument fetch comparison.
             */
            static int argfcmp(std::string& buff, char** argv, int argc, const char* cmp){
                for(int i = 0; i < argc; i++){
                    if(strcmp(argv[i], cmp) == 0){
                        if(i + 1 < argc){
                            buff = argv[i+1]; // I dunno if we have to resize buff before assigning, we'll see if it fails...
                            return 1;
                        }
                    }
                }
                return 0;
            }

            static int argfcmp(std::string& buff, char** argv, int argc, const char* cmp[], size_t cmp_size){
                for(int i = 0; i < cmp_size; i++){
                    for(int ii = 0; ii < argc; ii++){
                        if(strcmp(argv[ii], cmp[i]) == 0){
                            if(ii + 1 < argc){
                                buff = argv[ii + 1];
                                return 1;
                            }
                        }
                    }
                }
                return 0;
            }
    };

    class Utility{
        public:
            /**
             * @brief Hash files from an ifstream instance.
             * @param in ifstream
             * @return Hash of type size_t
             * @copyright Courtesy of Jet (C)
             */
            static inline const size_t hasher(std::ifstream& in){
                if(!in){throw std::runtime_error("invalid ifstream");}
                std::stringstream buffer;
                buffer << in.rdbuf();
                std::string content = buffer.str();
                std::hash<std::string> hash;
                return hash(content);
            }

            /**
             * @copyright Courtesy of Jet (C)
             */
            struct CaseInsensitiveCompare {
                bool operator()(const std::string& a, const std::string& b) const {
                    std::string lowerA = a;
                    std::string lowerB = b;
                    // Convert both strings to lowercase for comparison
                    std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(), ::tolower);
                    std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), ::tolower);
                    return lowerA < lowerB;
                }
            };

            static int replaceSubstr(std::string& str, const std::string& substr, const std::string& __rep){
                size_t pos = str.find(substr);
                if(pos == std::string::npos){
                    return 0;
                }
                str.replace(pos, substr.length(), __rep);
                return 1;
            }

            static const std::string generate_UUID(){
                boost::uuids::random_generator generator;
                const boost::uuids::uuid UUID = generator();
                return boost::uuids::to_string(UUID);
            }

            static const uint64_t timestamp(){
                std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
                const uint64_t t = std::chrono::system_clock::to_time_t(timePoint);
                
                return t;
            }

            static const uint64_t day_timestamp() {
                // Get the current time point
                std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
                // Convert to time_t (seconds since the Unix epoch)
                const uint64_t t = std::chrono::system_clock::to_time_t(timePoint);
                // Convert to days since the epoch, then back to seconds at the start of the day
                uint64_t dayTimestamp = (t / 86400) * 86400;
                return dayTimestamp;
            }

            static unsigned long long timestamp_ull(){
                std::chrono::time_point<std::chrono::system_clock> timePoint = std::chrono::system_clock::now();
                const unsigned long long t = std::chrono::system_clock::to_time_t(timePoint);
                
                return t;
            }

            template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
            static const std::string timestamp_to_str(const T& timestamp){
                static_assert(std::is_integral<T>::value);
                std::time_t ts = static_cast<std::time_t>(timestamp);
                std::tm* tm = std::localtime(&ts);
                
                char buffer[80];
                std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
                return std::string(buffer);
            }
            
            template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
            static const std::string timestamp_to_str_ymdhm(const T& timestamp){
                static_assert(std::is_integral<T>::value);
                std::time_t ts = static_cast<std::time_t>(timestamp);
                std::tm* tm = std::localtime(&ts);
                
                char buffer[80];
                std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm);

                return std::string(buffer);
            }

            template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
            static const T str_to_timestamp(const std::string& datetime) {
                std::tm tm = {};
                std::istringstream ss(datetime);
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                
                if (ss.fail()) {
                    ss.clear(); // Clear the error state
                    ss.str(datetime); // Reset the stringstream with the original input
                    // Try parsing without hour and minute
                    ss >> std::get_time(&tm, "%Y-%m-%d");
                    if (ss.fail()) {
                        return 0;
                    }
                }
                // Manually adjust tm_isdst to -1 to ignore DST adjustments
                tm.tm_isdst = -1;
                // Convert tm structure to time_point (assumes local time)
                auto time_c = std::chrono::system_clock::from_time_t(std::mktime(&tm));

                // Convert time_point back to time_t, then cast to T
                auto time_as_t = std::chrono::system_clock::to_time_t(time_c);

                // std::cout << "timestamp: " << time_as_t << std::endl;
                return static_cast<T>(time_as_t);  // Cast to desired type and return
            }

            template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
            static const std::string timestamp_to_str_ymd(const T& timestamp){
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



            void sortby_filename(std::vector<std::string>& filenames){
                std::sort(filenames.begin(), filenames.end(), [this](const std::string& path1, const std::string& path2){
                    std::string filename1 = std::filesystem::path(path1).filename().generic_string();
                    std::string filename2 = std::filesystem::path(path2).filename().generic_string();
                    return filename1 < filename2;
                });
            }

            std::string get_md5hash(const std::string& fname){ 
                char buffer[16384]; 
                unsigned char digest[MD5_DIGEST_LENGTH]; 

                std::stringstream ss; 
                std::string md5string; 

                std::ifstream ifs(fname, std::ifstream::binary); 

                MD5_CTX md5Context; 

                MD5_Init(&md5Context); 


                while(ifs.good()){ 
                    ifs.read(buffer, sizeof(buffer));
                    MD5_Update(&md5Context, buffer, ifs.gcount());
                } 

                ifs.close(); 

                int res = MD5_Final(digest, &md5Context); 
                    
                    if( res == 0 ) // hash failed 
                    return {};   // or raise an exception 

                // set up stringstream format 
                ss << std::hex << std::uppercase << std::setfill('0'); 


                for(unsigned char uc: digest){
                    ss << std::setw(2) << (int)uc;
                } 

                md5string = ss.str(); 

                return md5string; 
            }

            /* WARNING: this function cannot handle non-ASCII filenames and will throw a runtime error with fopen() */
            static inline std::string get_sha256hash(const std::string& fname){
                FILE *file;

                unsigned char buf[8192];
                unsigned char output[SHA256_DIGEST_LENGTH];
                size_t len;

                SHA256_CTX sha256;

                file = fopen(fname.c_str(), "rb");

                if(file == NULL){
                    const std::string err = "Error: Failed to open file to hash.\n";
                    throw std::runtime_error(err.c_str());
                }
                else{

                    SHA256_Init(&sha256);
                    while((len = fread(buf, 1, sizeof(buf), file)) != 0){
                        SHA256_Update(&sha256, buf, len);
                    } 
                    fclose(file);
                    SHA256_Final(output, &sha256);

                    std::stringstream ss;
                    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++){
                        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(output[i]);
                    }

                    return ss.str();
                }
            }

            static inline std::string get_sha256hash_cpp(std::ifstream& file) {
                // Buffer for reading from file
                unsigned char buf[8192];
                // Output SHA256 hash
                unsigned char output[SHA256_DIGEST_LENGTH];
                // SHA256 context
                SHA256_CTX sha256;
                
                if (!file.is_open()) {
                    const std::string err = "Error: Failed to open file to hash.\n";
                    throw std::runtime_error(err.c_str());
                }

                SHA256_Init(&sha256);
                // Read from the file stream
                while (file.read(reinterpret_cast<char*>(buf), sizeof(buf)) || file.gcount() != 0) {
                    SHA256_Update(&sha256, buf, file.gcount());
                }
                SHA256_Final(output, &sha256);

                // Convert hash to hexadecimal string
                std::stringstream ss;
                for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(output[i]);
                }

                return ss.str();
            }

            static inline std::string get_sha256hash_cpp(const std::string& fname) {
                // Convert fname to std::filesystem::path to handle special characters
                std::filesystem::path filepath(fname);

                // Open the file using std::ifstream with binary mode
                std::ifstream file(filepath, std::ios::binary);
                if (!file) {
                    const std::string err = "Error: Failed to open file to hash.\n";
                    throw std::runtime_error(err);
                }

                // Buffer for reading from file
                unsigned char buf[8192];
                // Output SHA256 hash
                unsigned char output[SHA256_DIGEST_LENGTH];
                // SHA256 context
                SHA256_CTX sha256;
                SHA256_Init(&sha256);

                // Read from the file stream
                while (file.read(reinterpret_cast<char*>(buf), sizeof(buf)) || file.gcount() != 0) {
                    SHA256_Update(&sha256, buf, static_cast<size_t>(file.gcount()));
                }
                file.close();

                SHA256_Final(output, &sha256);

                // Convert hash to hexadecimal string
                std::stringstream ss;
                for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(output[i]);
                }

                return ss.str();
            }

            
            static std::string ymd_date(){
                const std::chrono::time_point now(std::chrono::system_clock::now());
                const std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(now));

                std::stringstream ss;
                ss << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year()) << '-'
                << std::setw(2) << static_cast<unsigned>(ymd.month()) << '-'
                << std::setw(2) << static_cast<unsigned>(ymd.day());

                return ss.str();
            }        
            
            
            static char* ymd_date_cstyle(){
                char* out = (char *)malloc(11);
                std::time_t t=std::time(NULL);
                std::strftime(out, sizeof(out), "%Y-%m-%d", std::localtime(&t));
                
                return out;
            }
    };

    class Logs{
        private:
            static std::string logDir;
            static std::string logPath;
            static std::ofstream logf;
            
        public:
            static inline void init();

            static inline const std::string timestamp(){
                auto now = std::chrono::system_clock::now();
                std::string formatted_time = std::format("{0:%F_%Y-%M-%S}", now);

                return formatted_time;
            }


            static inline void log_cleaner(const std::string& dir, const unsigned& limit){

                std::vector<std::filesystem::path> fvec;
                for(const auto& x : std::filesystem::directory_iterator(dir)){
                    fvec.push_back(x);
                }

                if(fvec.size() > limit){
                    
                    std::sort(fvec.begin(), fvec.end());
                    try{
                        std::filesystem::remove(fvec[0]);
                        log_cleaner(dir, limit); // Check again
                    }
                    catch(std::filesystem::filesystem_error& err){
                        throw std::runtime_error(err);
                    }
                }
            }

            static inline std::ofstream create_log(const std::string& path){
                if(!std::filesystem::exists(logDir)){
                    std::filesystem::create_directories(logDir);
                }
                std::ofstream logf(logPath);
                if(!logf.is_open()){
                    throw std::runtime_error("Failed to open logfile");
                }
                log_cleaner(logDir, log_limit_extern);

                return logf;
            }

            static inline void msg(const std::string& message){
                const std::string msg = timestamp() + ":    " + message;
                logf << msg << std::endl;
            }

            ~Logs(){
                logf.close();
            }
    };

    class Filesystem {
        public:    
            /**
             * @brief Remove files and directories recursively
             * @param path Target
             * @note Can delete read only files. Permissions are modified before removal.
             */
            static inline void recurse_remove(const std::filesystem::path& path){
                for(const auto& entry : std::filesystem::recursive_directory_iterator(path)){
                    if(entry.is_directory()){
                        if(std::filesystem::is_empty(entry)){
                            std::filesystem::permissions(entry, std::filesystem::perms::all);
                            std::filesystem::remove(entry);
                        }
                        else{
                            recurse_remove(entry);
                        }
                    }
                    else{
                        std::filesystem::permissions(entry, std::filesystem::perms::all);
                        std::filesystem::remove(entry);
                    }
                }
            }

            /**
             * @brief Iterate recursively over a directory and pull all items into vector.
             * @param dirPath Target path as filesystem object
             * @param itemVector Vector of strings that the items get loaded into.
             * @note Self referencing function. Original object is modified.
             */
            static inline void recurse_scanner(const std::filesystem::path& dirPath, std::vector<std::string>& itemVector){
                for(const auto& entry : std::filesystem::directory_iterator(dirPath)){
                    if(entry.is_directory()){
                        recurse_scanner(entry.path(), itemVector);
                    }
                    else{
                        itemVector.push_back(entry.path().string());
                    }
                }
            }

            /**
             * @brief Recursively copy a directory or a single file
             * @param source Source path
             * @param destination Destination path
             * @param map Optional pass by reference map of strings. When defined, source is inserted as key with destination as value.
             * @note Optional parameter requires a map inside a reference_wrapper container, created in part with std::ref(normMap). normMap is modified and can be used like normal afterwards.
             */
            static int recurse_copy(const std::filesystem::path& source, const std::string& destination,
                                    std::optional<std::reference_wrapper<std::unordered_map<std::string, std::string>>> map = std::nullopt,
                                    std::optional<std::reference_wrapper<std::vector<std::pair<std::string,std::string>>>> pp = std::nullopt){

                if(std::filesystem::is_directory(source)){
                    for(const auto& entry : std::filesystem::directory_iterator(source)){
                        if(entry.is_directory()){
                            const std::string dst = destination + "\\" + entry.path().filename().string();
                            if(!std::filesystem::exists(dst)){
                                std::filesystem::create_directories(dst);
                            }
                            recurse_copy(entry.path(), dst, std::nullopt, pp);
                        }
                        else{
                            try{
                                if(!std::filesystem::exists(destination)){
                                    std::filesystem::create_directories(destination);
                                }
                                const std::string dstPath = destination + "\\" + entry.path().filename().string(); // full destination
                                std::filesystem::copy_file(entry.path(), dstPath, std::filesystem::copy_options::overwrite_existing);
                                if(map.has_value()){
                                    // Dereference the optional to access the map
                                    auto& mapDeref = map->get();
                                    mapDeref[entry.path().string()] = dstPath;
                                }
                                
                                if(pp.has_value()){
                                    // Dereference the optional to access the vector
                                    auto& ppDeref = pp->get();

                                    ppDeref.push_back(std::make_pair(entry.path().string(), dstPath));
                                }
                            }
                            catch(const std::filesystem::filesystem_error& err){
                                throw std::runtime_error(err.what());
                                return 0;
                            }
                        }
                    }
                }
                else{
                    try{
                        if(!std::filesystem::exists(destination)){
                            std::filesystem::create_directories(destination);
                        }
                        const std::string dstPath = destination + "\\" + std::filesystem::path(source).filename().string(); // full destination
                        std::filesystem::copy_file(source, dstPath, std::filesystem::copy_options::overwrite_existing);
                        if(map.has_value()){
                            auto& mapDeref = map->get(); // Dereference the optional
                            mapDeref[source.string()] = dstPath; // Add element to map
                        }
                        if(pp.has_value()){
                            // Dereference the optional to access the vector
                            auto& ppDeref = pp->get();
                            ppDeref.push_back(std::make_pair(source.string(), dstPath));
                        }
                    }
                    catch(const std::filesystem::filesystem_error& err){
                        throw std::runtime_error(err.what());
                        return 0;
                    }
                }

                
                return 1;
            }
    };

    class Serializer {
        public:
            static inline constexpr bool littleEndian(){
                return std::endian::native == std::endian::little;
            }
            
            // Revert endianness
            template <typename T>
            inline static T revert(T num){
                T res;
                for (std::size_t i = 0; i < sizeof(T); i++)
                    ((uint8_t *)&res)[i] = ((uint8_t *)&num)[sizeof(T) - 1 - i];
                return res;
            }

            // Serialize integers
            template <typename T, typename std::enable_if<std::is_integral<T>::value,T>::type* = nullptr>
            static inline void serialize(std::ofstream& of, T type){
                static_assert(std::is_integral<T>::value);   //check that T is of {char, int, ...} type
                static_assert(!std::is_reference<T>::value); //check that T is not a reference
                if(!of){throw std::runtime_error("invalid ofstream");}

                // if (littleEndian()){
                //     type = revert(type)
                // }
                of.write((char *)&type, sizeof(T));

            }

            // Overload for vector of strings
            static inline void serialize(std::ofstream& of, const std::vector<std::string>& v){
                if(!of){throw std::runtime_error("invalid ofstream");}
                size_t vSize = v.size();
                of.write((char *)&vSize, sizeof(vSize));
    
                for (const auto& elem : v){
                    size_t elemSize = elem.length();
                    of.write((char *)&elemSize, sizeof(elemSize));
                    of.write(elem.c_str(), elemSize);
                }
            }

            // Overload for strings
            static inline void serialize(std::ofstream& of, const std::string& str){
                if(!of){throw std::runtime_error("invalid ofstream");}
                size_t strLength = str.length();
                // if(littleEndian){
                //     strLength = revert(strLength);
                // }
                of.write((char *)&strLength, sizeof(strLength));
                of.write(str.data(), strLength);
            }

            // Overload for unordered_map of strings
            static inline void serialize(std::ofstream& of, const std::unordered_map<std::string,std::string>& map){
                if(!of){throw std::runtime_error("invalid ofstream");}
                size_t contElem = map.size();
                of.write((char *)&contElem, sizeof(contElem));
                
                for(const auto& [key,val] : map){
                    size_t keySz = key.size();
                    of.write((char *)&keySz, sizeof(keySz));
                    of.write(key.data(), keySz);

                    size_t valSz = val.size();
                    of.write((char *)&valSz, sizeof(valSz));
                    of.write(val.data(), valSz);
                }
            }
            
            // Deserialize integers
            template <typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
            static inline void deserialize(std::ifstream& in, T* type){
                static_assert(std::is_integral<T>::value);
                if(!in){throw std::runtime_error("invalid ifstream");}
                in.read((char *)type, sizeof(T));
                // if (littleEndian()){
                //     *type = revert(*type);
                // }
            }

            // Overload for strings
            static inline void deserialize(std::ifstream& in, std::string& str){
                if(!in){throw std::runtime_error("invalid ifstream");}
                size_t strLengthStack;
                size_t *strLength = &strLengthStack;
                in.read((char *)strLength, sizeof(strLength));
                // if(littleEndian){
                //     *strLength = revert(*strLength);
                // }
                str.resize(*strLength);
                in.read(str.data(), *strLength);
            }

            // Overload for vector of strings
            static inline void deserialize(std::ifstream& in, std::vector<std::string>& v){  
                if(!in){throw std::runtime_error("invalid ifstream");}
                size_t stackSize;
                size_t *size = &stackSize;
                in.read((char *)size, sizeof(size));
                for(unsigned i = 0; i < *size; i++){
                    size_t elemSizeStack;
                    size_t *elemSize = &elemSizeStack;
                    in.read((char *)elemSize, sizeof(elemSize));
                    // if(littleEndian()){
                    //     *elemSize = revert(*elemSize);
                    // }
                    std::string buff(*elemSize, '\0'); // Allocate string
                    in.read(buff.data(), *elemSize);
                    v.push_back(buff);
                }
            }

            static inline void deserialize(std::ifstream& in, std::unordered_map<std::string,std::string>& map){
                if(!in){throw std::runtime_error("invalid ifstream");}
                size_t contElem;
                size_t *contElemP = &contElem;
                in.read((char *)contElemP, sizeof(contElemP));
                for(unsigned i = 0; i < *contElemP; i++){
                    size_t keySz; 
                    size_t *keySzP = &keySz;
                    in.read((char *)keySzP, sizeof(keySzP));
                    std::string key(*keySzP, '\0');
                    in.read(key.data(), *keySzP);
                    
                    size_t valSz;
                    size_t *valSzP = &valSz;
                    in.read((char *)valSzP, sizeof(valSzP));
                    std::string val(*valSzP, '\0');
                    in.read(val.data(), *valSzP);
                    map[key] = val;
                }
            }

            template <typename T, typename = void>
            struct has_save_traits : std::false_type {};

            template <typename T>
            struct has_save_traits<T, std::void_t<
                decltype(std::declval<T>().userName),
                decltype(std::declval<T>().driveLetter),
                decltype(std::declval<T>().message),
                decltype(std::declval<T>().pathVec)
                >> : std::integral_constant<bool,
                std::is_same<decltype(std::declval<T>().userName), std::string>::value &&
                std::is_same<decltype(std::declval<T>().driveLetter), std::string>::value &&
                std::is_same<decltype(std::declval<T>().message), std::string>::value &&
                std::is_same<decltype(std::declval<T>().pathVec), std::vector<std::pair<std::string, std::string>>>::value>
            {};

            template <typename T, typename std::enable_if<has_save_traits<T>::value, int>::type = 0> 
            static inline void serialize(std::ofstream& of, std::unordered_map<std::string, std::map<uint64_t, T>>& m){
                if(!of){throw std::runtime_error("invalid ofstream");}
                size_t mSize = m.size();
                of.write((char *)&mSize, sizeof(mSize));
                for(const auto& pair : m){
                    size_t keySz = pair.first.length();
                    of.write((char *)&keySz, sizeof(keySz));
                    of.write(pair.first.c_str(), keySz);
                    size_t nestmapSz = pair.second.size();
                    of.write((char *)&nestmapSz, sizeof(nestmapSz));
                    for(const auto& nestpair : pair.second){
                        of.write((char *)&nestpair.first, sizeof(nestpair.first));
                        size_t unameSz = nestpair.second.userName.length();
                        of.write((char *)&unameSz, sizeof(unameSz));
                        of.write(nestpair.second.userName.c_str(), unameSz);
                        size_t driveSz = nestpair.second.driveLetter.length();
                        of.write((char *)&driveSz, sizeof(driveSz));
                        of.write(nestpair.second.driveLetter.c_str(), driveSz);
                        size_t messageSz = nestpair.second.message.length();
                        of.write((char *)&messageSz, sizeof(messageSz));
                        of.write(nestpair.second.message.c_str(), messageSz);
                        size_t pathvecSz = nestpair.second.pathVec.size();
                        of.write((char *)&pathvecSz, sizeof(pathvecSz));
                        for(const auto& pathpair : nestpair.second.pathVec){
                            size_t pathASz = pathpair.first.length();
                            of.write((char *)&pathASz, sizeof(pathASz));
                            of.write(pathpair.first.c_str(), pathASz);
                            size_t pathBSz = pathpair.second.length();
                            of.write((char *)&pathBSz, sizeof(pathBSz));
                            of.write(pathpair.second.c_str(), pathBSz);
                        }
                    }
                }
            }

            template <typename T, typename std::enable_if<has_save_traits<T>::value, int>::type = 0>
            static inline void deserialize(std::ifstream& in, std::unordered_map<std::string, std::map<uint64_t, T>>& m){
                if(!in){throw std::runtime_error("invalid ifstream");}
                size_t contElem;
                size_t *contElemP = &contElem;
                in.read((char *)contElemP, sizeof(contElem));
                for(unsigned i = 0; i <  *contElemP; i++){
                    size_t keySzStack;
                    size_t *keySz = &keySzStack;
                    in.read((char *)keySz, sizeof(keySzStack));
                    std::string key(*keySz, '\0');
                    in.read(key.data(), *keySz);
                    size_t nestmapSzStack;
                    size_t *nestmapSz = &nestmapSzStack;
                    in.read((char *)nestmapSz, sizeof(nestmapSzStack));
                    for(unsigned ii = 0; ii < *nestmapSz; ii++){
                        uint64_t ullStack;
                        uint64_t *ull = &ullStack;
                        in.read((char *)ull, sizeof(ullStack));
                        size_t unameSzStack;
                        size_t *unameSz = &unameSzStack;
                        in.read((char *)unameSz, sizeof(unameSzStack));
                        std::string uname(*unameSz, '\0');
                        in.read(uname.data(), *unameSz);
                        size_t driveSzStack;
                        size_t *driveSz = &driveSzStack;
                        in.read((char *)driveSz, sizeof(driveSzStack));
                        std::string drive(*driveSz, '\0');
                        in.read(drive.data(), *driveSz);
                        size_t messageSzStack;
                        size_t *messageSz = &messageSzStack;
                        in.read((char *)messageSz, sizeof(messageSzStack));
                        std::string message(*messageSz, '\0');
                        in.read(message.data(), *messageSz);
                        size_t pathvecSzStack;
                        size_t *pathvecSz = &pathvecSzStack;
                        in.read((char *)pathvecSz, sizeof(pathvecSzStack));
                        std::vector<std::pair<std::string,std::string>> vec;
                        for(unsigned iii = 0; iii < *pathvecSz; iii++){
                            size_t pathASzStack;
                            size_t *pathASz = &pathASzStack;
                            in.read((char *)pathASz, sizeof(pathASzStack));
                            std::string pathA(*pathASz, '\0');
                            in.read(pathA.data(), *pathASz);
                            size_t pathBSzStack;
                            size_t *pathBSz = &pathBSzStack;
                            in.read((char *)pathBSz, sizeof(pathBSzStack));
                            std::string pathB(*pathBSz, '\0');
                            in.read(pathB.data(), *pathBSz);
                            vec.push_back(std::make_pair(pathA,pathB));
                        }
                        T save = {uname, drive, message, vec};
                        m[key][*ull] = save;
                    }
                }
            }

    };

    class Programs {
        public:
            struct PInfo{
                std::string name;
                std::vector<std::string> paths;
                std::vector<std::string> procNames;
                PInfo() = default;
                PInfo(const std::string& name, const std::vector<std::string> paths, const std::vector<std::string> procNames)
                    : name{name}, paths{paths}, procNames{procNames} {} 
            };

            class Mgm {
                private:
                    std::unordered_map<std::string, PInfo> _programs;
                    std::unordered_map<std::string, std::string> aliasMap;
                    std::set<std::string, CS::Utility::CaseInsensitiveCompare> sup;
                public:
                    Mgm(){
                        add("Jackett", {"C:\\ProgramData\\Jackett"}, {"JackettTray.exe", "JackettConsole.exe"});
                        setAlias("Jackett", {"jackett"});
                        
                        add("Prowlarr", {"C:\\ProgramData\\Prowlarr"}, {"Prowlarr.Console.exe"});
                        setAlias("Prowlarr", {"prowlarr"});
                        
                        add("qBittorrent", {"C:\\Users\\" + uName + "\\AppData\\Local\\qBittorrent", "C:\\Users\\" + uName + "\\AppData\\Roaming\\qBittorrent"}, {"qBittorrent.exe"});
                        setAlias("qBittorrent", {"qbittorrent"});
                        
                        add("JDownloader", {"C:\\Users\\" + uName + "\\AppData\\Local\\JDownloader 2.0\\cfg"}, {"JDownloader2.exe"});
                        setAlias("JDownloader", {"jdownloader", "Jdownloader", "jDownloader"});
                        
                        add("Deemix", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\deemix"}, {"win-x64-latest.exe", "deemix-gui.exe"});
                        setAlias("Deemix", {"deemix"});
                        
                        add("Microsoft.PowerToys", {"C:\\Users\\" + uName + "\\AppData\\Local\\Microsoft\\PowerToys"}, {"PowerToys.exe"});
                        setAlias("Microsoft.PowerToys", {"powertoys", "PowerToys"});
                        
                        add("Microsoft.VisualStudioCode", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\Code\\User\\settings.json", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Code\\User\\keybindings.json", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Code\\User\\tasks.json"}, {"Code.exe"});
                        setAlias("Microsoft.VisualStudioCode", {"VisualStudioCode", "VsCode", "vscode", "Vscode", "VScode", "VSCODE", "Visual-Studio-Code", "visual-studio-code", "VSCode"});
                        
                        add("Ultimaker.Cura", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\cura\\" + get_cura_vers()}, {"UltiMaker-Cura.exe", "CuraEngine.exe"});
                        setAlias("Ultimaker.Cura", {"CuraSlicer", "UltimakerCura", "Ultimaker-Cura", "Ultimaker.Cura", "ultimaker.cura", "Cura", "Cura-Slicer", "Cura-slicer", "cura"});

                        add("Fusion360", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\Autodesk\\Neutron Platform\\Options\\" + get_fusion360_dir()}, {"Fusion360.exe", "ADPClientService.exe", "AdskIdentityManager.exe"});
                        setAlias("Fusion360", {"Autodesk.Fusion360", "fusion360", "autodesk.fusion360", "autodesk-fusion360", "Autodesk-Fusion360"});
                        
                        add("Google.Chrome", {"C:\\Users\\" + uName + "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Preferences"}, {"chrome.exe"});
                        setAlias("Google.Chrome", {"GoogleChrome", "googlechrome", "Google-Chrome", "google-chrome", "Google.Chrome", "google.chrome", "chrome", "Chrome"});

                        add("Mozilla.Firefox", get_firefox_paths(), {"firefox.exe"});
                        setAlias("Mozilla.Firefox", {"MozillaFirefox", "mozillafirefox", "Firefox", "firefox", "mozilla.firefox", "Mozilla-Firefox", "mozilla-firefox"});
                        
                        add("CMD-Macros", {"C:\\Data\\" + uName + "\\Software\\Settings\\cmd"}, {});
                        setAlias("CMD-Macros", {"cmd-macros", "cmd.macros", "CMD.Macros", "cmdmacros"});
                        
                        add("Powershell-Macros", {"C:\\Data\\" + uName + "\\Software\\Settings\\Powershell"}, {});
                        setAlias("Powershell-Macros", {"powershell-macros", "powershell.macros", "Powershell.Macros", "powershellmacros", "pwsh-macros", "pwsh.macros", "Pwsh.Macros", "Pwsh-Macros"});
                        
                        add("Calibre", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\calibre"}, {"calibre.exe", "calibre-parallel.exe"});
                        setAlias("Calibre", {"calibre"});
                        
                        add("Mp3tag", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\Mp3tag"}, {"Mp3tag.exe"});
                        setAlias("Mp3tag", {"mp3tag", "MP3Tag", "MP3tag", "Mp3Tag"});

                        add("foobar2000-v2", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\foobar2000-v2"}, {"foobar2000.exe"});
                        setAlias("foobar2000-v2", {"foobar2000", "foobar2000v2"});
                        
                        add("Chatterino", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\Chatterino2\\Settings"}, {"chatterino.exe"});
                        setAlias("Chatterino", {"chatterino"});

                        add("OBS-Studio", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\obs-studio"}, {"obs64.exe"});
                        setAlias("OBS-Studio", {"OBS", "obs-studio", "obs", "OBS-studio"});
                        
                        add("Elgato-StreamDeck", {"C:\\Users\\" + uName + "\\AppData\\Roaming\\Elgato\\StreamDeck\\Audio", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Elgato\\StreamDeck\\Backup", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Elgato\\StreamDeck\\Data", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Elgato\\StreamDeck\\IconPacks", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Elgato\\StreamDeck\\Marketplace", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Elgato\\StreamDeck\\Plugins", "C:\\Users\\" + uName + "\\AppData\\Roaming\\Elgato\\StreamDeck\\ProfilesV2"}, {"StreamDeck.exe"});
                        setAlias("Elgato-StreamDeck", {"StreamDeck", "streamdeck", "stream-deck", "elgato-stream-deck", "elgato-streamdeck", "Elgato-StreamDeck"});
                        
                        add("Steam", get_steam_paths(), {"steam.exe", "steamwebhelper.exe", "mist.exe", "GameOverlayUI.exe", });
                        setAlias("Steam", {"Valve.Steam", "valve.steam", "steam"});

                        for(const auto& pair : _programs){
                            sup.insert(pair.first);
                        }
                    }

                    inline void add(const std::string& name, const std::vector<std::string>& paths, const std::vector<std::string>& procNames){
                        _programs[name] = PInfo(name, paths, procNames);
                    }

                    inline void setAlias(const std::string& canonical, const std::vector<std::string>& aliases){
                        for(const auto& el : aliases){
                            aliasMap[el] = canonical;
                        }
                    }

                    inline const std::set<std::string, CS::Utility::CaseInsensitiveCompare>& get_supported(){
                        return sup;
                    }

                    inline int checkName(const std::string& name){
                        if(_programs.find(name) != _programs.end()){
                            return 1;
                        }
                        else if(aliasMap.find(name) == aliasMap.end()){
                            return 0;
                        }
                        return 1;
                    }

                    inline std::string get_canonical(const std::string& name){
                        if(_programs.find(name) != _programs.end()){
                            return name;
                        }
                        auto aliasIt = aliasMap.find(name);
                        if(aliasIt != aliasMap.end()){
                            return aliasIt->second;
                        }
                        return {};
                    }

                    inline std::unordered_map<std::string,PInfo>& programs(){
                        return _programs;
                    }
                    
                    /**
                     * @brief Get current installed version of Cura Slicer
                     * @param userName Username
                     * @note only versions above 4.0 will be returned 
                     */
                    const std::string get_cura_vers(){
                        const std::string roamingPath = "C:\\Users\\" + uName + "\\AppData\\Roaming\\cura\\";
                        unsigned i = 0; // matches
                        std::string ret;
                        if(std::filesystem::exists(roamingPath) && !std::filesystem::is_empty(roamingPath)){
                            for(const auto& entry : std::filesystem::directory_iterator(roamingPath)){
                                if(std::filesystem::is_directory(entry.path())){
                                    const std::string fname = entry.path().filename().string();
                                    try{
                                        if(std::stof(fname) >= 4.0){ // Check version
                                            i++;
                                            ret = fname;
                                        }
                                    }
                                    catch(const std::invalid_argument& err){
                                        continue;
                                    }
                                }
                            }
                            if(i <= 1){
                                return ret;
                            }
                        }
                        return {};
                    }

                    inline const std::string get_fusion360_dir(){
                        const std::string roamingPath = "C:\\Users\\" + uName + "\\AppData\\Roaming\\Autodesk\\Neutron Platform\\Options";
                        unsigned i = 0;
                        std::string ret;
                        if(!std::filesystem::exists(roamingPath)){
                            return {};
                        }
                        for(const auto& entry : std::filesystem::directory_iterator(roamingPath)){
                            if(entry.is_directory()){
                                ret = entry.path().filename().string();
                                i++;
                            }
                        }

                        if(i <= 1){
                            return ret;
                        }
                        else{
                            std::cerr << "This should never happen. Please report this issue to https:\\\\github.com\\TeeJay69" << std::endl;
                        }

                        return {};
                    }
                    
                    inline const std::string find_firefox_prefs_file(const std::filesystem::__cxx11::directory_entry& __entry){
                        if(__entry.is_directory()){
                            for(const auto& entry : std::filesystem::directory_iterator(__entry)){
                                if(entry.is_directory()){
                                    find_firefox_prefs_file(entry);
                                }
                                else{
                                    if(entry.path().filename().string() == "prefs.js"){
                                        return entry.path().string();
                                    }
                                }
                            }
                        }
                        else{
                            if(__entry.path().filename().string() == "prefs.js"){
                                return __entry.path().string();
                            }
                        }

                        return {};
                    }

                    inline const std::vector<std::string> get_firefox_paths(){
                        const std::string roamingPath = "C:\\Users\\" + uName + "\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles";
                        std::vector<std::string> ret;
                        if(std::filesystem::exists(roamingPath)){
                            for(const auto& entry : std::filesystem::directory_iterator(roamingPath)){
                                const std::string& pref = find_firefox_prefs_file(entry);
                                if(!pref.empty()){
                                    ret.push_back(pref);
                                }
                            }
                        }
                        return ret;
                    }

                    std::vector<std::string> get_steam_paths() {
                        std::vector<std::string> ret;
                        // Add the fixed paths
                        ret.push_back("C:\\Users\\" + uName + "\\AppData\\Local\\Steam\\htmlcache\\LocalPrefs.json");
                        ret.push_back("C:\\Users\\" + uName + "\\AppData\\Local\\Steam\\htmlcache\\UserPrefs.json");
                        ret.push_back("C:\\Program Files (x86)\\Steam\\config\\config.vdf");
                        ret.push_back("C:\\Program Files (x86)\\Steam\\config\\libraryfolders.vdf");

                        const std::string userdataPath = "C:\\Program Files (x86)\\Steam\\userdata";

                        if (std::filesystem::exists(userdataPath) && std::filesystem::is_directory(userdataPath)) {
                            for (const auto& userDirEntry : std::filesystem::directory_iterator(userdataPath)) {
                                if (userDirEntry.is_directory()) {
                                    // This is the "someNumber" directory
                                    std::filesystem::path configPath = userDirEntry.path() / "config";
                                    if (std::filesystem::exists(configPath) && std::filesystem::is_directory(configPath)) {
                                        // Check for "grid" directory
                                        std::filesystem::path gridPath = configPath / "grid";
                                        if (std::filesystem::exists(gridPath) && std::filesystem::is_directory(gridPath)) {
                                            ret.push_back(gridPath.string());
                                        }
                                        // Check for the .vdf files
                                        std::vector<std::string> vdfFiles = {
                                            "shortcuts.vdf",
                                            "serverbrowser_ui.vdf",
                                            "localconfig.vdf"
                                        };
                                        for (const auto& vdfFile : vdfFiles) {
                                            std::filesystem::path filePath = configPath / vdfFile;
                                            if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
                                                ret.push_back(filePath.string());
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        return ret;
                    }

            };
    };

    class Organizer {
        public:
            // Limit number of saves inside the config archive.
            static void limit_configarchive(const int& limit, const std::string& path){
                std::vector<std::filesystem::path> itemList;
                
                for(const auto& item : std::filesystem::directory_iterator(path)){ // Iterate over date dirs
                    if(item.is_directory()){

                        // Check if directory begins with "2"
                        if(item.path().filename().generic_string()[0] == '2'){
                            
                            itemList.push_back(item.path());
                        }
                    }
                }
                
                // Logic for removing the oldest save
                if(itemList.size() > limit){
                    std::sort(itemList.begin(), itemList.end());
                    
                    try{
                        CS::Filesystem::recurse_remove(itemList[0]); // remove oldest save
                        limit_configarchive(limit, path); // Self reference
                    }
                    catch(std::filesystem::filesystem_error& error){
                        std::cerr << ANSI_COLOR_RED << "Failed to remove directory. (Class: organizer). Error Code: 38 & <" << ">" << ANSI_COLOR_RESET << std::endl;
                        throw std::runtime_error(error.what());
                    }
                }
            }
    };
    
    class Saves {
        private:
            struct InternalSave {
                std::string userName;
                std::string driveLetter;
                std::string message;
                std::vector<std::pair<std::string,std::string>> pathVec;
                InternalSave() = default;
                InternalSave(const std::string& username, const std::string& drive, const std::string& message, const std::vector<std::pair<std::string,std::string>>& paths)
                 : userName {username}, driveLetter {drive}, message {message}, pathVec {paths} {}
            };
            const std::string _file;
            std::unordered_map<std::string, std::map<uint64_t, InternalSave>> _saves;

        public:
            Saves(const std::string& fpath) : _file(fpath) {}

            struct PublicSave{
                std::string program;
                std::string username;
                std::string driveletter;
                std::string message;
                std::vector<std::pair<std::string,std::string>> pathvec;
            };

            inline int load(){
                std::ifstream in(_file, std::ios_base::binary);
                if(!in){
                    return 0;
                }
                CS::Serializer::deserialize(in, _saves);
                if(_saves.empty()){
                    return 0;
                }
                in.close();
                return 1;
            }

            inline int save(){
                const std::string parDir = std::filesystem::path(_file).parent_path().string();
                if(!std::filesystem::exists(parDir)){
                    std::filesystem::create_directories(parDir);
                }

                std::ofstream of(_file, std::ios_base::binary);
                if(!of){
                    return 0;
                }
                CS::Serializer::serialize(of, _saves);
                of.close();
                return 1;
            }

            inline void add(const std::string& program, const std::string& username, const std::string& driveletter,  const std::string& message, const std::vector<std::pair<std::string,std::string>>& pathvec, const uint64_t& tst){
                _saves[program][tst] = InternalSave(username, driveletter, message, pathvec);
            }

            inline void add(const std::string& program, const std::string& username, const std::string& drive, const std::string& message, const std::vector<std::pair<std::string,std::string>>& pathvec){
                _saves[program][CS::Utility::timestamp()] = InternalSave(username, drive, message, pathvec);
            }

            inline void add(const PublicSave& ps_){
                _saves[ps_.program][CS::Utility::timestamp()] = InternalSave(ps_.username, ps_.driveletter,  ps_.message, ps_.pathvec);
            }

            inline int erase_program(const std::string& prog){
                if(_saves.erase(prog) != 1){
                    return 0;
                }
                return 1;
            }

            inline int erase_save(const std::string& prog, const uint64_t& tst){
                if(_saves.at(prog).erase(tst) != 1){
                    return 0;
                }
                return 1;
            }

            inline int find_save(const std::string& prog, const uint64_t& tst){
                if(_saves[prog].find(tst) == _saves[prog].end()){
                    return 0;
                }
                return 1;
            }

            inline uint64_t get_last_tst(const std::string& prog){
                if(_saves.find(prog) == _saves.end()){
                    return 0;
                }

                return _saves.at(prog).rbegin()->first;
            }

            inline InternalSave get_lastsave(const std::string& prog){
                if(_saves.find(prog) == _saves.end()){
                    return {};
                }

                return _saves.at(prog).rbegin()->second;
            }

            inline uint64_t get_oldest_tst(const std::string& prog){
                return _saves.at(prog).begin()->first;
            }

            inline InternalSave get_oldestsave(const std::string& prog){
                return _saves.at(prog).begin()->second;
            }

            inline int exists(const std::string& prog){
                if(_saves.find(prog) == _saves.end()){
                    return 0;
                }

                return 1;
            }

            inline const std::unordered_map<std::string, std::map<uint64_t, InternalSave>>& saves(){
                return _saves;
            }

            inline int cmp_root(const std::string& __prog, const uint64_t __date, const std::string __root){
                if(_saves.at(__prog).at(__date).driveLetter != __root){
                    return 0;
                }

                return 1;
            }

            inline int replace_root(const std::string& __prog, const uint64_t __date, const std::string __root){
                size_t pos = _saves.at(__prog).at(__date).pathVec[0].second.find(":");
                std::string substr = _saves.at(__prog).at(__date).pathVec[0].second.substr(pos - 1, 2);

                for(std::pair<std::string,std::string>& pair : _saves.at(__prog).at(__date).pathVec){
                    if(pos == std::string::npos){
                        return 0;
                    }
                    CS::Utility::replaceSubstr(pair.second, substr, __root);
                }
                
                return 1;
            }

            inline int cmp_uname(const std::string& __prog, const uint64_t __date, const std::string& uname){
                if(_saves.at(__prog).at(__date).userName != uName){
                    return 0;
                }
                return 1;
            }

            inline void replace_uname(const std::string& __prog, const uint64_t __date, const std::string& uname){
                for(std::pair<std::string,std::string>& pair : _saves.at(__prog).at(__date).pathVec){
                    const std::string pat = "Users\\";
                    const size_t pos = pair.first.find(pat);
                    if(pos != std::string::npos){
                        const size_t posx = pos + pat.length();
                        const size_t posend = pair.first.find_first_of("\\", posx);
                        pair.first.replace(posx, posend - posx, uname);

                    }
                    const size_t pos1 = pair.second.find(pat);
                    if(pos1 != std::string::npos){
                        const size_t posEndPat = pos1 + pat.length();
                        const size_t posMatchEnd = pair.second.find_first_of("\\", posEndPat);
                        pair.second.replace(posEndPat, posMatchEnd - posEndPat, uname);
                    }
                }
            }
    };

    class Task {
        public:
            /**
             * @brief Creates a new task in the windows task scheduler.
             * @param taskname The name for the scheduled task.
             * @param target The file path to the target of the task.
             * @param taskfrequency The intervall and frequency of the scheduled task. Ftring format: '<daily/hourly>,<value>'
             * @return 1 for success. 0 for error.
             */
            static const int create(const std::string& taskname, const std::string& target, const std::string& taskfrequency){
                
                std::stringstream ss(taskfrequency);
                std::string token;

                const std::string timeframe;
                const std::string intervall;
                std::vector<std::string> list;

                while(getline(ss, token, ',')){
                    list.push_back(token);
                }

                const std::string com = "cmd /c \"schtasks /create /tn \"" + taskname + "\" /tr \"\\\"" + target + "\\\"sync --all\" /sc " + list[0] + " /mo " + list[1] + " >NUL 2>&1\"";

                if(std::system(com.c_str()) != 0){
                    return 0;
                }

                return 1;
            }


            /**
             * @brief Removes a task from the windows task scheduler.
             * @param taskname The name as identifier for the task.
             * @return 1 for success. 0 for error.
             */
            static const int remove(const std::string& taskname){

                const std::string com = "cmd /c \"schtasks /delete /tn \"" + taskname + "\" /F >NUL 2>&1\"";

                if(std::system(com.c_str()) != 0){
                    return 0;
                }

                return 1;
            }

            /**
             * @brief Status check for scheduled tasks.
             * @param taskname The name as identifier for the task.
             * @return 1 if the task exists, 0 if it does not exist.
             */
            static const int exists(const std::string& taskname){

                const std::string com = "cmd /c \"schtasks /query /tn " + taskname + " >NUL 2>&1\"";

                if(std::system(com.c_str()) != 0){
                    return 0;
                }

                return 1;
            }
    };

    class Process{
        public:
            static int findPID(const char *procname) {

                HANDLE hSnapshot;
                PROCESSENTRY32 pe;
                int pid = 0;
                BOOL hResult;

                // snapshot of all processes in the system
                hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

                // initializing size: needed for using Process32First
                pe.dwSize = sizeof(PROCESSENTRY32);

                // info about first process encountered in a system snapshot
                hResult = Process32First(hSnapshot, &pe);

                // retrieve information about the processes
                // and exit if unsuccessful
                while (hResult) {
                    // if we find the process: return process ID
                    if (strcmp(procname, (const char*)(pe.szExeFile)) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                    }  
                    hResult = Process32Next(hSnapshot, &pe);
                }

                // closes an open handle (CreateToolhelp32Snapshot)
                CloseHandle(hSnapshot);
                return pid;
            }
            
            static int killPid(int pid){
                if(pid == 0){
                    return 0;
                }
                const HANDLE adpservice = OpenProcess(PROCESS_TERMINATE, false, pid);
                TerminateProcess(adpservice, 1);
                CloseHandle(adpservice);
                
                return 1;
            }

            /**
             * @brief Kill a process by its name
             * @param processName Process name
             * 
             */
            static int killProcess(const char* processName){
                const int x = findPID(processName);
                if(x == 0){
                    return 0;
                }
                const auto adpservice = OpenProcess(PROCESS_TERMINATE, false, x);
                TerminateProcess(adpservice, 1);
                CloseHandle(adpservice);
                
                return 1;
            }
    };
}
#endif