# ConfigSync Development Plan



## Minimal description:
 - Makes restoring the configuration/settings of programs easier when reinstalling them or when moving to a new system etc
    - bckup Compress-Log
    - Cura Slicer
    - JDownloader
    - qBittorrent
    - Jackett
        - Backup whole install in ProgramData\Jackett
    - Prowlarr
    - Microsoft PowerToys
    - Fusion360
    - Chrome
    - Firefox



## Feature Dev:

- ~~Per program stores config in extra directory by date.~~
- ~~Checks if config changed.~~
    - ~~If it did, keeps the previous config folder and creates a new one with current date~~
    - ~~If didnt change, rename the folder to current date~~
- ~~Keeps x number of previous config folders when x = currentNumber, delete oldest folder~~
- ~~config file lists programs for which a config backup will be created // needed?~~
- ~~programs.hpp class for program specific paths/info~~

- Command line options:
    + sync [program name]--default:all.
        - Save a programs config.
        - Defaults to all supported programs.
        
    + restore/rollback [program name] ([date to restore]--default:newest).
        - Rollback a programs config by replacing it with a save from the archive.
        - defaults to most recent save.
        - --All restores all programs config

    + show [program name]--default:all
        - Show all saves
        - Show number of saves 


    + status [program name]--default:all
        - Check if config is up to date.
        - Display list of the programs, along with their last save date
        - If program specified, display last save date, along with if the config is up to date.
        

    + settings
        - --show-default, shows default settings.
        - --json, opens config file
        - 
        - Scheduled task On/Off
            - No intervall provided: default:xxx
        - Change Intervall of scheduled task
        - Show never synced apps in 'status'
    
    + list
        - displays all supported programs
    
    + version
    + help
    
        
- Service/ScheduledTask
    - Logic for creating a scheduled task
    - Logic for deciding/displaying if scheduled task should be created

- Installer
    - Create an installer
    - Keep saved configs
        - Update structure if needed
        - Notify when new version is not backward compatible. 
            - Ask if user is ok with removing the saves.
            - Option to move current install to configsync.old
    - Update

- Support running program through explorer

- Option to install on a USB stick, so that the archive is on the flash drive and restoring on a new pc is easier.



When your application initializes:

Load the existing configuration file, regardless of its version.
If the version does not match what's current for the application:
Create a new blank property tree for the new configuration
For each node in the tree, have a set of expected property names, and a function pointer or similar to get this setting if it's absent in the old file's tree. If a setting changes its format, give it a new name.
Search for each setting in the old tree, copying it if it's found, and using the result of the supplied function if it's not.
Save your new settings file.
Your "missing setting" functions can:

Return a constant default value.
Query the tree for a different setting and convert it (with a default value if the old setting isn't found either)
Ask the user


When copying a backup is created in temp which is then moved to recycle bin. We could skip the temp step and move directly to recycle bin.
Then we create 

- Structure:

Install-Location
    |---ConfigArchive
    |    |---Program-1
    |    |    |---Save-1 (DateDir)
    |    |    |---Save-2 (DateDir)
    |    |
    |    |---Program-2
    |       |---Save-1 (DateDir)
    |       |---Save-2 (DateDir)
    |
    |---ConfigBackup
    |    |---Program-1
    |    |   |---Temp
    |    |   |   |---287421 (UUID)
    |    |   |
    |    |   |---RecycleBin
    |    |       |---928811
    |    |       |---118272
    |    |       |---327366
    |    |
    |    |---Program-2
    |
    <!-- |---UndoHistory
        |
        |---Program-1
            |
            |---878920
            |---893742 -->



## Critical ToDo's:
- [x] Fix hardcoded absolute program location problem. Program needs the absolute location where its stored on the filesystem, not where its run from!
- [x] Check that we keep the copy of the config (because of restore) after program has run --> When restoring we backup to configbackup\programname\temp\. When restoring we copy contents of temp to configbackup\programname\recyclebin.
- [x] Cleanup recyclebin after x
- [x] Verify that save dir system supports multiple saves on the same day.
- [x] Check if a save exists before restoring 
- [x] Add cleanup to sync and restore parts.
- [x] Support restore on machine that has a different user name (Paths from maps...)
- [x] Add log file
- [x] Catch `Ctrl + C`
- [ ] Add logging
- [ ] Format --help message.
- [ ] License stuff. Include license in installer. Display license in help message.
- [ ] Add more verbose messages.
- [ ] Add disclaimer for runas admin before restoring. (Folder access) (something like, may require running from elevated command prompt)
- [ ] Support pointing to existing Archive during installation (Copy existing Archive to installation directory).
- [ ] Support CMake
- [ ] Doxygen comments
- [ ] Refactor the way the last save dir is retrieved. A unique Id linked to the date might be helpful.

- 

## Reevaluate:

// TODO: Make database path copy function self referencing with error message parameter. Use only this function wherever you copy database paths
- Sync argument shouldnt default to all programs. 'sync --all' for synchronizing all programs
- Message 'Config is in sync' instead of 'Config is up to date'
- Display last save for every program, 'cfgs status' --default 
- Change 'Rollback complete' to 'Successfully restored <program name>' or 'Restored <programname> successfully'
- [ ] Add id to hashbase file.


## Feature requests:

- [ ] Implement incremental config saves, (storing modified files in vector)
- [ ] Write the integer map in database.hpp serialized to a binary file. 
- [ ] Let user change number of saves to keep
- [ ] Let user delete saves
- [ ] Option to change default behavior of restore param (Disable auto default to newest save, instead show save dates for a program. Let user select the date that he wants to restore)
- [ ] Option to identify specific save date with an alias (Like '12.November')
- [ ] Finer control over task frequency
- [ ] Verbose message showing old setting vs new setting when changing settigns
- [ ] Backup settings file, in case primary gets corrupted or messed with.
- [ ] Verify settings file using hash function.
- [ ] --force option for sync (Make new snapshot regardless of hashes)
- [ ] Status indicator for is_identical() function from analyzer
- [ ] --verify parameter to make an integrity check of a snapshot from the archive. (defaults to newest) (option to provide specific date) ([PROGRAM] --all for verifying all saves of that program.) (verify --all checks every single save from all programs.)(--all-newest for checking the newest save from all programs in the archive)

## Trashcan:
<!--  // Fetch Index of backups
                auto IX = anly.get_Index("Jackett", exePath);
    
                std::cout << ANSI_COLOR_222 << "Showing Jackett:" << ANSI_COLOR_RESET << std::endl;
                std::cout << ANSI_COLOR_166 << "Snapshots: " << ANSI_COLOR_RESET << std::endl;
                unsigned i = 1;
                for(const auto& save : saveList){ // Show all saves
                    std::cout << ANSI_COLOR_146 << i << ". " << save << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                
                // Show restore backups:
                std::cout << ANSI_COLOR_166 << "Restore backups: " << ANSI_COLOR_RESET << std::endl;
                i = 0;
                for(const auto& pair : IX.time_uuid){
                    std::cout << ANSI_COLOR_151 << i << ". " << synchronizer::timestamp_to_string(pair.first) << ANSI_COLOR_RESET << std::endl;
                    i++;
                }
                std::cout << "Found " << saveList.size() << " saves and " << i << " Restore backups" << std::endl; -->
<!-- /* void backup_config_for_restore(const std::string& dirUUID){
            std::string backupDir = exeLocation + "\\ConfigBackup\\" + programName;

            // Check if backup directory is empty
            if(std::filesystem::is_empty(backupDir)){
                // Create backup directory in ConfigBackup folder
                std::filesystem::create_directories(backupDir + "\\temp");
                std::filesystem::create_directories(backupDir + "\\RecycleBin");
            }
            
            // Create UUID dir in temp
            try{
                std::filesystem::create_directory(backupDir + "\\temp\\" + dirUUID);
            }
            catch(std::filesystem::filesystem_error& errorCode){
                throw std::runtime_error(errorCode);
            }

            // Create Path file inside UUID dir 
            std::string databasePath = backupDir + dirUUID + "\\" + "ConfigSync-PathDatabase.bin";
            std::map<std::string, std::string> pathMap;
            std::optional<std::reference_wrapper<std::map<std::string, std::string>>> mapRef = std::ref(pathMap);

            // Backup to UUID directory
            std::unordered_map<std::string, std::string> id;
            int groupFlag = 0;
            if(programconfig::has_groups(programName)){ // Check for groups
                programconfig pcfg(programName, exeLocation);
                id = pcfg.get_path_groups();
                groupFlag = 1;
            }
            else{
                groupFlag = 0;
            }

            // Copy to UUID dir
            for(const auto& item : programPaths){
                std::string groupName;

                if(std::filesystem::is_directory(item)){

                    if(groupFlag == 1 && id.contains(item)){
                        groupName = id[item];
                    }
                    else{
                        groupName = "Directories";
                    }

                    std::string destination = backupDir + dirUUID + "\\" + groupName;
                    try{
                        recurse_copy(item, destination, mapRef);
                    }
                    catch(cfgsexcept& err){
                        std::cerr << err.what() << std::endl;
                    }
                }
                else{

                    if(groupFlag == 1 && id.contains(item)){
                        groupName = id[item];
                    }
                    else{
                        groupName = "Single-Files";
                    }

                    std::string destination = backupDir + dirUUID + "\\" + groupName;
                    try{
                        recurse_copy(item, destination, mapRef);
                    }
                    catch(cfgsexcept& err){
                        std::cerr << err.what() << std::endl;
                    }
                }
            }

            // Write path map to file

            database db(databasePath);
            db.storeStringMap(pathMap);
        } */ -->
<!--         static void timestamp_path_map(std::map<unsigned long long, std::string>& map, const std::string& strValue){
            std::chrono::time_point<std::chrono::system_clock> timePoint;
            timePoint = std::chrono::system_clock::now();
            unsigned long long t = std::chrono::system_clock::to_time_t(timePoint);
            
            map[t] = strValue;
        } -->
<!--  if(!std::filesystem::is_empty(std::filesystem::path(backupDir + "\\temp"))){ // Clean up temp directory
                database db(backupDir + "\\RecycleBin\\RecycleMap.bin"); // Inside respective programs RecycleBin dir

                for(const auto& item : std::filesystem::directory_iterator(backupDir + "\\temp")){
                    const std::string newPath = backupDir + "\\RecycleBin\\" + item.path().filename().string();
                    std::filesystem::rename(item, newPath); // Move item to recyclebin
                    timestamp_objects(recycleMap, newPath); // Timestamp and load into map                                       
                }

                // Clean recyclebin
                const std::string mapPath = backupDir + "\\RecycleBin\\RecycleMap.bin";
                organizer janitor;
                janitor.recyclebin_cleaner(recyclebinlimit, recycleMap, mapPath); // Clean recycle bin and store recycle map as file
            } -->
<!--       /*
        void hash_program_items(std::unordered_map<std::string, std::string>& hashMap){
            std::vector<std::string> vec;
            get_config_items_current(vec);

            for(const auto& entry : vec){
                const std::string& md5 = get_md5hash(entry);
                hashMap[entry] = md5;
            }
        }


        void hash_saved_items(std::unordered_map<std::string, std::string>& hashMap){
            std::vector<std::string> vec;
            get_config_items_saved(vec);

            for(const auto& entry : vec){
                const std::string& md5 = get_md5hash(entry);
                hashMap[entry] = md5;
            }
        }
        */ -->
<!-- if(!std::filesystem::exists(hbPath)){
                logf << logs::ms("THIS SHOULD NEVER HAPPEN!!! the hashmap should always exist before this function (is_identical()) is called!!" + __LINE__);

                for(const auto& [pathA, pathB] : pathMap){
                    // TODO: Multithread the hashing
                    const std::string hashA = s(pathA);
                    const std::string hashB = get_md5hash(pathB);

                    if(hashA != hashB){
                        return 0;
                    }
                    
                    h.ha.push_back(hashA);
                    h.hb.push_back(hashB);
                    h.pa.push_back(pathA);
                    h.pb.push_back(pathB);
                }

                std::ofstream of(hbPath);
                of.close();
                database::storeHashbase(hbPath, h);
            }
                 -->
<!-- int is_identical_config(){ // Main function of this class
            
            // Get saved config items
            std::vector<std::string> configItemsSaved;
            get_config_items_saved(configItemsSaved);
            sortby_filename(configItemsSaved);
            std::cout << "items saved 0,1: " << configItemsSaved[0] << "  1: " << configItemsSaved[1] << std::endl;
            
            // Get ProgramPaths
            std::vector<std::string> configItemsCurrent;
            get_config_items_current(configItemsCurrent);
            sortby_filename(configItemsCurrent);
            std::cout << "items current 0,1: " << configItemsCurrent[0] << "  1: " << configItemsCurrent[1] << std::endl;
            
            // Compare vectors
            if(is_identical_filenames(configItemsSaved, configItemsCurrent)){
                int i = 0;
                for(const auto& item : configItemsSaved){
                    
                    if(!is_identical_bytes(item, configItemsCurrent[i])){
                        std::cout << "FILES NOT IDENTICAL!!!: " << item << configItemsCurrent[i] << std::endl;
                        return 0; // config changed
                    }
                    
                    i++;
                }
            }
            else{
                std::cerr << "this is a problem!" << std::endl;
                return 0;
            }

            std::cout << "Debug a169\n";
            return 1; // config did not change
        } -->
<!-- - Combine show and status arguments' -->
<!-- The following commands are available -->
<!-- 
restore config:
    - ~~Check if current config exists~~
    - ~~Create backup of current config:~~
        - ~~move any existing items inside the ConfigBackup\\program\\temp directory to ConfigBackup\\program\\RecycleBin~~
        - ~~copy C:\\program\\configPath to programs ConfigBackup\\program\\temp directory, name with UUID or pseudo random value.~~
    - ~~Copy each directory in ConfigArchive\\program\\newestDate\\ to its corresponding C:\\program\\configPath directory. Overwrite existing files.~~
    - On Error, 
        - ~~copy each directory in ConfigBackup\\program\\temp to its corresponfing C:\\program\\configPath (using UUID). Overwrite existing files.~~
        - ~~move backup inside the ConfigBackup\\program\\temp directory to ConfigBackup\\program\\RecycleBin~~
    - On Success: 
        - ~~move backup inside the ConfigBackup\\program\\temp directory to ConfigBackup\\program\\RecycleBin~~
        - ~~If RecycleBin has more than x number of items, delete x number of oldest items in RecycleBin directory.~~
        - ~~Inform user that config was restored successfully~~ -->
<!-- // if(std::filesystem::is_directory(source)){
                //     std::cout << "Debug s109\n";
                //     std::cout << "source debug s111: " << source << std::endl;
                //     std::cout << "destination debug s112: " << destination << std::endl;
                //     try{
                //         // std::filesystem::copy(source, std::filesystem::path(destination), std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
                //         std::cout << "Debug s113\n";
                //     }
                //     catch(std::filesystem::filesystem_error& copyError){
                //         std::cout << "Debug s117\n " << copyError.what();
                //         return 0; // throw std::runtime_error(copyError);
                //     }

                    // Store path pair in map
                //     std::cout << "Debug s122\n";
                //     pathMap.insert(std::make_pair(item, destination));
                // }
                // else{
                //     std::string sourceParentDir = source.parent_path().filename().string();
                //     std::string destination = archivePathAbs + "\\" + dateDir + "\\" + sourceParentDir + "\\" + source.filename().string();
                //     std::cout << "Debug s128\n";

                //     try{
                //         std::filesystem::create_directories(archivePathAbs + "\\" + dateDir + "\\" + sourceParentDir);
                //         std::cout << "Debug s132\n";
                //         std::filesystem::copy(source, std::filesystem::path(destination), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
                //         std::cout << "Debug s134\n";
                //     }
                //     catch(std::filesystem::filesystem_error& copyError){
                //         std::cout << "Debug s137\n";
                //         return 0; // throw std::runtime_error(copyError);
                //     }

                    // std::cout << "Debug s141\n";
                    // // Store path pair in map
                    // pathMap.insert(std::make_pair(item, destination));
                    // std::cout << "Debug s144\n";
                // }
                // std::cout << "Debug s146\n"; -->
                

## Parking Lot
<!-- else if(hb.ha.empty() && hb.hb.empty() && !hb.pa.empty() && !hb.pb.empty() && hb.hh.empty() && hb.pp.empty()){
                for(unsigned i = 0; i < hb.ha.size(); i++){
                    hf << '0' << "," << "0" << "," << hb.pa[i] << "," << hb.pb[i] << "\n";
                }
            }
            else if(!hb.ha.empty() && !hb.hb.empty() && !hb.pa.empty() && !hb.pb.empty() && hb.hh.empty() && hb.pp.empty()){
                for(unsigned i = 0; i < hb.ha.size(); i++){
                    hf << hb.ha[i] << "," << hb.hb[i] << "," << hb.pa[i] << "," << hb.pb[i] << "\n";
                }
            } -->
<!-- 

            // std::string groupName;
            // if(programconfig::has_groups(programName)){
            //     programconfig pcfg(programName, exeLocation);
            //     groupName = pcfg.get_path_groups();
            // }
            // recurse_scanner(std::filesystem::path(lastSavedDir) / std::filesystem::path("Directories"), configItems);
            
            // const std::filesystem::path singleFiles = std::filesystem::path(lastSavedDir) / std::filesystem::path("Single-Files");
            // if(std::filesystem::exists(singleFiles)){
            //     recurse_scanner(singleFiles, configItems);

            // }
            // /* Get pathmap */
            // database db(lastSavedDir + "\\ConfigSync-PathDatabase.bin");
            // std::map<std::string, std::string> map;
            // db.readStringMap(map);

            // for(const auto& pair : map){
            //     configItems.push_back(pair.second);
            // }

  int is_identical_config(){ // Main function of this class
            
            // Get saved config items
            std::vector<std::string> configItemsSaved;
            get_config_items_saved(configItemsSaved);
            sortby_filename(configItemsSaved);
            std::cout << "items saved 0,1: " << configItemsSaved[0] << "  1: " << configItemsSaved[1] << std::endl;
            
            // Get ProgramPaths
            std::vector<std::string> configItemsCurrent;
            get_config_items_current(configItemsCurrent);
            sortby_filename(configItemsCurrent);
            std::cout << "items current 0,1: " << configItemsCurrent[0] << "  1: " << configItemsCurrent[1] << std::endl;
            
            // Compare vectors
            if(is_identical_filenames(configItemsSaved, configItemsCurrent)){
                int i = 0;
                for(const auto& item : configItemsSaved){
                    
                    if(!is_identical_bytes(item, configItemsCurrent[i])){
                        std::cout << "FILES NOT IDENTICAL!!!: " << item << configItemsCurrent[i] << std::endl;
                        return 0; // config changed
                    }
                    
                    i++;
                }
            }
            else{
                std::cerr << "this is a problem!" << std::endl;
                return 0;
            }

            std::cout << "Debug a169\n";
            return 1; // config did not change
        }



        void get_config_items_saved(std::vector<std::string>& configItems){
            std::cout << "Debug a110\n";
            const std::string lastSavedDir = get_newest_backup_path();
            std::cout << "lastSavedDir: " << lastSavedDir << std::endl;
            std::cout << "Debug a112\n";
            const std::filesystem::path configPathSaved = lastSavedDir;
            if(std::filesystem::exists(configPathSaved)){
                std::cout << "configPathSaved exists!!!\n";
            }
            std::cout << "configPathSaved: " << configPathSaved << std::endl;
            
            recurse_scanner(configPathSaved, configItems);
            std::cout << "Debug a116\n";
        
        }

        
    std::cout << "ConfigSync (JW-Coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::cout << "usage: cfgs [OPTIONS]... [PROGRAM]\n" << std::endl;
        std::cout << std::endl;
        std::cout << "The following commands are available:" << std::endl;
        std::cout << "sync [PROGRAM]            Synchronize configuration files of the specified program.";
        std::cout << "sync --all                Synchronize configuration files of all supported programs.";
        std::cout << "restore [PROGRAM] [DATE]  Restore configuration files of the specified program, date.";
        std::cout << "restore --all             Restore configuration files of all supported programs.";
        std::cout << "status [PROGRAM]          Display synchronization state. (Default: all)";
        

        
    else if(std::string(argv[1]) == "help" || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"){ // Help message param
        std::cout << "ConfigSync (JW-Coreutils) " << VERSION << std::endl;
        std::cout << "Copyright (C) 2024 - Jason Weber" << std::endl;
        std::cout << "The Config-Synchronizer utility enables syncing and restoring of programs configuration files.\n" << std::endl;
        std::cout << "usage: cfgs [command] [options]\n" << std::endl;
        std::cout << "The following commands are available:" << std::endl;
        std::cout << "sync [program]        Synchronize a program."
        std::exit(EXIT_SUCCESS);



    if(argc <= 1){
        std::cout << "This program is currently CLI only.\nPlease open a command prompt or powershell window and run it from there.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        std::exit(EXIT_SUCCESS);
    }
    // @section program run through explorer
    // Get programconfig
    programconfig jackett("Jackett", exePath.generic_string());
    std::vector<std::string> jackettPaths = jackett.getFilePaths();

    programconfig prowlarr("Prowlarr", exePath.generic_string());
    std::vector<std::string> prowlarrPaths = prowlarr.getFilePaths();

    programconfig qbittorrent("qBittorrent", exePath.generic_string());
    std::vector<std::string> qbittorrentPaths = qbittorrent.getFilePaths();


    if(std::filesystem::exists(jackett.get_archive_path())){
        // something
    }
    else{
        std::filesystem::create_directories(jackett.get_archive_path());
        // Make first backup
    }

    if(std::filesystem::exists(prowlarr.get_archive_path())){
        // 
    }
    else{
        std::filesystem::create_directories(prowlarr.get_archive_path());
    }

    if(std::filesystem::exists(qbittorrent.get_archive_path())){
        // 
    }
    else{
        std::filesystem::create_directories(qbittorrent.get_archive_path());
    }

+++
if(path_check(jackettPaths) == 1){ //Program paths verified
                analyzer configAnalyzer(jackettPaths, "Jackett", exePath);

                if(configAnalyzer.is_identical_config()){ // Config identical    
                    std::cout << "Jackett config did not change, updating save date..." << std::endl;

                    std::filesystem::path newestPath = configAnalyzer.get_newest_backup_path();
                    std::filesystem::rename(newestPath, newestPath.parent_path().string() + "\\" + synchronizer::ymd_date()); // Update name of newest save dir

                }
                else{ // Config changed
                    std::cout << "Jackett config changed.\nSynchronizing Jackett..." << std::endl;
                    synchronizer sync(jackettPaths, "Jackett", exePath);

                    if(sync.copy_config(jackett.get_archive_path().string(), synchronizer::ymd_date()) == 0){
                        std::cerr << "Error synchronizing Jackett." << std::endl;
                    }
                }
            }
 -->
-------------------------------------------------