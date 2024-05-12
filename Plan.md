# ConfigSync Development Plan

## Minimal description:
 - Software Configuration Synchronizer

## Commit guidelines:
### Format:
> The mood should be whatever feels natural and most appropriate. One can use the imparative for the headline and the present for the body if that feels right.
```Git
type(scope): Headline
-> Description

type(scope): Headline
-> Description

<!-- Example -->
feat(list): Display installed programs
-> 'list --installed' operand (-i). Shows the date and the name of the program. The output is sorted in ascending order of date.
```

### Types:
* **feat**: A new feature
* **fix**: A bug fix
* **docs**: Documentation only changes
* **style**: Changes that do not affect the meaning of the code (white-space, formatting, missing
semi-colons, etc)
* **refactor**: A code change that neither fixes a bug nor adds a feature
* **perf**: A code change that improves performance
* **test**: Adding missing or correcting existing tests
* **chore**: Changes to the build process or auxiliary tools and libraries such as documentation
generation

## Feature Dev:
- Command line options:
    + sync [PROGRAM]
        - [x] --all (-a) (synchronize all supported programs)
        - Operands:
            - [x] --message (-m) (Add a note to that save)
            - [x] --force (-f) (Kill running instances and skip up-to-date check)
    + restore [PROGRAM] (Defaults to latest save)
        - [x] --all (-a) (restore config of all supported programs)
        - Operands:
            - [x] --date (-d) (specify a specific save. Uses save from that date or if unavailble, the latest save before that date)
            - [x] --force (-f) (Force restore by killing running instances of the target programs)
    + check
        - [x] --task (-t) (ensure that task status reflects settings)
    + show [PROGRAM] (Display saves)
        - [x] --explorer (-e) (Open save archive in explorer)
    + list
        - [x] List all supported programs
    + undo [PROGRAM]
        - [x] --restore (-r) (Undo the most recent restore of a program)
        - [x] --save (-s) (Undo the most recent save of a program)
        - Operands:
            - [x] --all (-a) (Undo an action for all programs)
            - [x] --date (-d) (Undo a specific action from a specific date)
            - [x] --force (-f) (Forced restore, kill running instances of target program)
    + status [PROGRAM]->default:all
    + settings
        - [x] --reset (-r) [SETTING] (Reset a specific setting to default)
            - [x] --all (-a) (Resets all settings to default)
        - [x] --json (-j) (View raw settings file)
        - [x] settings.savelimit
        - [x] settings.pre-restore-limit
        - [x] settings.task
        - [x] settings.taskfrequency
        - [x] settings.editor (vim, notepad, VSCode) (Preferred editor when opening files from this program.)
    + [ ] remove (delete saves/restores)
    + version
    + help
        - [x] --programs (-p) (Display information about specific programs)
- Operands:
    + source (e.g. winget, choco, jet/direct (repository of official releases from websites) / directly from website)
    + --disable-auto-packman-install (Prevents automatic installation of package managers like winget and chocolatey, when not installed)
    + --silent (-s) skip confirmation
- Structure:

Install-Location
    |---config
    |    |---xxx
    |
    |---registers
    |    |---xxx
    |    |---xxx
    |
    |---index
        |---NameTable


## Critical ToDo's:
- [ ] Block "Pre-Restore-Backup" as --message value for syncing.
- [ ] Recognize operands in any order for all options

## Reevaluate:
- [Declined] ~~xxx~~
- [x] xxx

## Feature requests:
- [Declined] ~~xxx~~
- [ ] xxx

## Known Bugs:
- [Closed] xxx
- Previously synchronized save. error when trying to hash the original file. (forced sync worked, then non forced may encounter issues, with potential file locks preventing hashing.) Mp3tag

## Trashcan:
<!-- code -->


## Parking Lot
<!-- CS::Saves S(savesFile);
            S.load();
            uint64_t tst = S.get_last_tst(canName);
            uint64_t daytst = CS::Utility::day_timestamp();
            uint64_t 
            if(tst != 0 && daytst < tst && tst <= daytst + 86400){
                S.erase_save(canName, tst);    
            } -->
<!-- std::cout << "Please enter a name for the register\n" << std::flush;
                                    bool nameLoop = true;
                                    while(nameLoop != false){
                                        std::string name;
                                        std::getline(std::cin, name);
                                        if(name.empty()){
                                            continue;
                                        }
                                        else if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_-") != std::string::npos){
                                            std::cout << ANSI_COLOR_RED << "Invalid characters in name. Please try again." << ANSI_COLOR_RESET << std::endl;
                                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                        }
                                        else{
                                            newReg.version = VERSION;
                                            newReg.uuid = Jet::Registers::generate_UUID();
                                            newReg.timestamp = Jet::Registers::timestamp();
                                            newReg.lastUsed = Jet::Registers::timestamp();
                                            newReg.name = name;
                                            std::cout << "Register members:\n" << std::flush;
                                            for(const auto& elem : newReg.vec){
                                                std::cout << elem << std::endl;
                                            }
                                            std::cout << "Register name: " << newReg.name << std::endl;
                                            std::cout << "uuid: " << newReg.uuid << std::endl;
                                            std::cout << "timestamp: " << newReg.timestamp << std::endl;
                                            std::cout << "lastUsed: " << newReg.lastUsed << std::endl;

                                            std::ofstream of(exeLoc + "\\registers\\" + newReg.uuid);
                                            Jet::Serializer::serialize(of, newReg);
                                            of.close();

                                            std::cout << "New register was added!" << std::endl;
                                            Jet::Registers::Reg dR;
                                            std::ifstream in(exeLoc + "\\registers\\" + newReg.uuid);
                                            Jet::Serializer::deserialize(in, dR);
                                            std::cout << "Debug: " << dR.version << "\n"
                                            << "dR.uuid: " << dR.uuid << "\n"
                                            << "dR.timestamp: " << dR.timestamp << "\n"
                                            << "dR.lastUsed: " << dR.lastUsed << "\n"
                                            << "dR.vec[0]: " << dR.vec[0] << "\n"
                                            << "dR.vec[1]: " << dR.vec[1] << "\n"
                                            << "dR.name: " << dR.name << "\n" << std::flush;
                                            std::exit(EXIT_SUCCESS);
                                        }
                                    } -->
<!--   
    // const uint64_t serivers = 1; // Serializer version
    
    // /**
    //  * @brief Template for serializing integral data types
    //  * @param data Integral value
    //  */
    // template <typename T>
    // void serialize(std::ofstream& out, const T& data){
    //     static_assert(std::is_integral<T>::value, "optional message");
    //     // Serializer version;
    //     out.write(reinterpret_cast<const char*>(&serivers), sizeof(serivers));

    //     // Write data
    //     out.write(reinterpret_cast<const char*>(&data), sizeof(T));
    // }
    
    // /**
    //  * @brief Overload for strings
    //  * @param data String 
    //  */
    // template <>
    // void serialize(std::ofstream& out, const std::string& data){
    //     // Serializer version;
    //     out.write(reinterpret_cast<const char*>(&serivers), sizeof(serivers));
        
    //     // Write size of data
    //     const uint64_t dsize = static_cast<uint64_t>(data.length());
    //     out.write(reinterpret_cast<const char*>(&dsize), sizeof(dsize));

    //     // Write data
    //     out.write(data.data(), dsize);
    // }

    // // Overload for vectors
    // template <typename T>
    // void serialize(std::ofstream& out, std::vector<T>& vec){
    //     // Serializer version
    //     out.write(reinterpret_cast<const char*>(&serivers), sizeof(serivers));

    //     // Element num
    //     const uint64_t vecSize = vec.size();
    //     out.write(reinterpret_cast<const char*>(&vecSize), sizeof(vecSize));

    //     // const auto t1 = std::chrono::high_resolution_clock::now();
    //     for(const auto& el : vec){
    //         // elem length
    //         const uint64_t dsize = el.length();
    //         out.write(reinterpret_cast<const char*>(&dsize), sizeof(dsize));
            
    //         // write elem
    //         out.write(el.data(), dsize);
    //     }
    //     // const auto t2 = std::chrono::high_resolution_clock::now();
    //     // const auto duration = t2 - t1;
    //     // const auto conv =  std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
    //     // std::cout << "Vector serialized in: " << conv << "\n";
    // }
    
    
    // /**
    //  * @brief Template for deserializing integral data types
    //  * @param in Input stream
    //  * @param value Integral
    //  */
    // template <typename T>
    // void deserialize(std::ifstream& in, T& value){
    //     static_assert(std::is_integral<T>::value, "optional message");
    //     // serializer 
    //     uint64_t serial;
    //     in.read(reinterpret_cast<char*>(&serial), sizeof(serial));
    //     if(serial != serivers){
    //         return;
    //     }
    //     // read value
    //     in.read(reinterpret_cast<char*>(&value), sizeof(T));
    // }

    // /**
    //  * @brief Overload for string types
    //  * @param in Input stream
    //  * @param data String
    //  */
    // template <>
    // void deserialize(std::ifstream& in, std::string& data){
    //     // serializer version
    //     uint64_t serial;
    //     in.read(reinterpret_cast<char*>(&serial), sizeof(serial));
    //     if(serial != serivers){
    //         return;
    //     }

    //     // data size
    //     uint64_t dsize;
    //     in.read(reinterpret_cast<char*>(&dsize), sizeof(dsize));
        
    //     // data
    //     data.resize(dsize);
    //     in.read(data.data(), dsize);
    // }

    // /**
    //  * @brief Overload for vectors with multipe types
    //  * @param in Input stream
    //  * @param data String
    //  */
    // template <typename T>
    // void deserialize(std::ifstream& in, std::vector<T>& vec){
    //     // version
    //     uint64_t serial;
    //     in.read(reinterpret_cast<char*>(&serial), sizeof(serial));
    //     if(serial != serivers){
    //         return;
    //     }
    //     // elem num
    //     uint64_t elnum;
    //     in.read(reinterpret_cast<char*>(&elnum), sizeof(elnum));
    //     vec.resize(elnum);
    //     for(unsigned i = 0; i < elnum; i++){
    //         // read elem size
    //         uint64_t elsize;
    //         in.read(reinterpret_cast<char*>(&elsize), sizeof(uint64_t));
    //         // resize vector
    //         vec[i].resize(elsize);
    //         // Read element
    //         in.read(vec[i].data(), elsize);
    //     }
    // } -->
<!--  Link for AppInstaller-Installer msixbundle https://aka.ms/getwinget -->
<!-- code -->
<!--   template <typename T>
    void dsrlint(char* buffer, T& val){
        static_assert(std::is_integral<T>(), "Type must be an integral type");
        const size_t size = sizeof(T);
        for(unsigned i = 0; i < size; i++){
            // val |= (buffer << (i * 8));
        }
    }
 -->
 <!--  /**
     * @brief Serialize integral types
     * @param buff char* buffer to store serialized value
     * @param val value to serialize
     */ 
    template <typename T>
    void srlint(char* buff,  const T& val){
        static_assert(std::is_integral<T>(), "Type must be an integral type");
        const size_t size = sizeof(T);
        for(unsigned i = 0; i < size; i++){
            // Shifts the value val to the right by i * size bits and performs a bitwise AND operation with 0xFF to isolate each byte. 
            buff[i] = (val >> (i * 8)) & 0xFF;
        }
    } -->