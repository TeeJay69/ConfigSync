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
        |---Program-1
        |   |---Temp
        |   |   |---287421 (UUID)
        |   |
        |   |---RecycleBin
        |       |---928811
        |       |---118272
        |       |---327366
        |
        |---Program-2

            

## Critical ToDo's:
- [x] Fix hardcoded absolute program location problem. Program needs the absolute location where its stored on the filesystem, not where its run from!
- [x] Check that we keep the copy of the config (because of restore) after program has run --> When restoring we backup to configbackup\programname\temp\. When restoring we copy contents of temp to configbackup\programname\recyclebin.
- [x] Cleanup recyclebin after x
- [x] Verify that save dir system supports multiple saves on the same day.
- [x] Check if a save exists before restoring 
- [x] Add cleanup to sync and restore parts.
- [x] Support restore on machine that has a different user name (Paths from maps...)
- [ ] Format --help message.
- [ ] License stuff. Include license in installer. Display license in help message.
- [ ] Add more verbose messages.
- [ ] Add disclaimer for runas admin before restoring. (Folder access) (something like, may require running from elevated command prompt)
- [ ] Support pointing to existing Archive during installation (Copy existing Archive to installation directory).
- [ ] Support CMake
- [ ] Add logging
- [ ] Catch `Ctrl + C`
- [ ] Doxygen comments



## Reevaluate:

// TODO: Make database path copy function self referencing with error message parameter. Use only this function wherever you copy database paths
- Sync argument shouldnt default to all programs. 'sync --all' for synchronizing all programs
- Message 'Config is in sync' instead of 'Config is up to date'
- Display last save for every program, 'cfgs status' --default 
- Change 'Rollback complete' to 'Successfully restored <program name>' or 'Restored <programname> successfully'



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

## Trashcan:

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



## Parking Lot
<!-- 
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