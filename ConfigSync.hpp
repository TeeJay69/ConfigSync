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

extern int verbose;
extern const std::string uName;
extern const int log_limit_extern;
extern std::string pLocat;
extern const std::string sessionStamp;

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
                std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm);

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
                char buffer[4096]; 
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


            std::string get_sha256hash(const std::string& fname){
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
            // Call only once in your program!
            inline void init(){
                logDir = pLocat + "\\logs";
                logPath = logDir + "\\" + timestamp();
                logf = create_log(logPath);
            }
            
            inline const std::string timestamp(){
                auto now = std::chrono::system_clock::now();
                std::string formatted_time = std::format("{0:%F_%Y-%M-%S}", now);

                return formatted_time;
            }


            inline void log_cleaner(const std::string& dir, const unsigned& limit){

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

            inline std::ofstream create_log(const std::string& path){
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

            inline void msg(const std::string& message){
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
                        recurse_remove(entry);
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
                                    std::optional<std::reference_wrapper<std::map<std::string, std::string>>> map = std::nullopt,
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
                decltype(std::declval<T>().pathMap)
                >> : std::integral_constant<bool,
                std::is_same<decltype(std::declval<T>().userName), std::string>::value &&
                std::is_same<decltype(std::declval<T>().driveLetter), std::string>::value &&
                std::is_same<decltype(std::declval<T>().pathMap), std::unordered_map<std::string, std::string>>::value>
            {};

            template <typename T, typename std::enable_if<has_save_traits<T>::value, int>::type = 0> 
            static inline void serialize(std::ofstream& of, std::unordered_map<std::string, std::unordered_map<uint64_t, T>>& m){
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
                        size_t pathmapSz = nestpair.second.pathMap.size();
                        of.write((char *)&pathmapSz, sizeof(pathmapSz));
                        for(const auto& pathpair : nestpair.second.pathMap){
                            size_t pathKeySz = pathpair.first.length();
                            of.write((char *)&pathKeySz, sizeof(pathKeySz));
                            of.write(pathpair.first.c_str(), pathKeySz);
                            size_t pathValSz = pathpair.second.length();
                            of.write((char *)&pathValSz, sizeof(pathValSz));
                            of.write(pathpair.second.c_str(), pathValSz);
                        }
                    }
                }
            }

            template <typename T, typename std::enable_if<has_save_traits<T>::value, int>::type = 0>
            static inline void deserialize(std::ifstream& in, std::unordered_map<std::string, std::unordered_map<uint64_t, T>>& m){
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
                        size_t pathmapSzStack;
                        size_t *pathmapSz = &pathmapSzStack;
                        in.read((char *)pathmapSz, sizeof(pathmapSzStack));
                        for(unsigned iii = 0; iii < *pathmapSz; iii++){
                            size_t pathKeySzStack;
                            size_t *pathKeySz = pathKeySzStack;
                            in.read((char *)pathKeySz, sizeof(pathKeySzStack));
                            std::string pathKey(*pathKeySz, '\0');
                            in.read(pathKey.data(), *pathKeySz);
                            size_t pathValSzStack;
                            size_t *pathValSz = &pathValSzStack;
                            in.read((char *)pathValSz, sizeof(pathValSzStack));
                            std::string pathVal(*pathValSz, '\0');
                            in.read(pathVal.data(), *pathValSz);
                            T save = {uname, drive, {{pathKey, pathVal}}};
                            m[key] = {{ull, save}};
                        }
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
                    std::unordered_map<std::string, PInfo> programs;
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

                        for(const auto& pair : programs){
                            sup.insert(pair.first);
                        }
                    }

                    inline void add(const std::string& name, const std::vector<std::string>& paths, const std::vector<std::string>& procNames){
                        programs[name] = PInfo(name, paths, procNames);
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
                        if(programs.find(name) != programs.end()){
                            return 1;
                        }
                        else if(aliasMap.find(name) == aliasMap.end()){
                            return 0;
                        }
                        return 1;
                    }

                    inline std::string get_canonical(const std::string& name){
                        if(programs.find(name) != programs.end()){
                            return name;
                        }
                        auto aliasIt = aliasMap.find(name);
                        if(aliasIt != aliasMap.end()){
                            return aliasIt->second;
                        }
                        return {};
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
                        for(const auto& entry : std::filesystem::directory_iterator(roamingPath)){
                            if(std::filesystem::is_directory(entry.path())){
                                const std::string fname = entry.path().filename().string();
                                if(std::stof(fname) >= 4.0){ // Check version
                                    i++;
                                    ret = fname;
                                }
                            }
                        }
                        if(i <= 1){
                            return ret;
                        }
                        return {};
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
                std::unordered_map<std::string, std::string> pathMap;
                InternalSave() = default;
                InternalSave(const std::string& username, const std::string& drive, const std::unordered_map<std::string, std::string>& paths) : userName {username}, driveLetter {drive}, pathMap {paths} {}
            };
            const std::string _file;
            std::unordered_map<std::string, std::unordered_map<uint64_t, InternalSave>> _saves;

        public:
            Saves(const std::string& fpath) : _file(fpath) {}

            struct PublicSave{
                std::string program;
                std::string username;
                std::string driveletter;
                std::unordered_map<std::string, std::string> pathmap;
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

            inline void add(const std::string& program, const uint64_t& tst, const std::string& username, const std::string& driveletter, const std::unordered_map<std::string, std::string>& pathmap){
                _saves[program][tst] = InternalSave(username, driveletter, pathmap);
            }

            inline void add(const std::string& program, const std::string& username, const std::string& drive, const std::unordered_map<std::string,std::string>& pathmap){
                _saves[program][CS::Utility::timestamp()] = InternalSave(username, drive, pathmap);
            }

            inline void add(const PublicSave& ps_){
                _saves[ps_.program][CS::Utility::timestamp()] = InternalSave(ps_.username, ps_.driveletter, ps_.pathmap);
            }

            inline int erase_program(const std::string& prog){
                if(_saves.erase(prog) != 1){
                    return 0;
                }
                return 1;
            }

            inline int erase_save(const std::string& prog, const uint64_t& tst){
                if(_saves[prog].erase(tst) != 1){
                    return 0;
                }
                return 1;
            }
    };
    class Process{
        public:

            int findPID(const char *procname) {

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
                    if (strcmp(procname, pe.szExeFile) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                    }  
                    hResult = Process32Next(hSnapshot, &pe);
                }

                // closes an open handle (CreateToolhelp32Snapshot)
                CloseHandle(hSnapshot);
                return pid;
            }

            /**
             * @brief Kill a process by its name
             * @param processName Process name
             * 
             */
            int killProcess(const char* processName){

                const int x = findPID(processName);
                
                if(x == NULL || x == 0){
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