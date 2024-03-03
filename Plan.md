# ConfigSync Development Plan


-------------------------------------------------.
# Minimal description:
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

-------------------------------------------------.

-------------------------------------------------.
# Feature Dev:

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

    + show [program name]
        - Show last save date
        - Show number of saves

    + status

    + settings
        - Scheduled task On/Off 
            - No intervall provided: default:xxx
        - Change Intervall of scheduled task
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

- Support running program through explorer

- Option to install on a USB stick, so that the archive is on the flash drive and restoring on a new pc is easier.

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
-------------------------------------------------
            
-------------------------------------------------
# Critical ToDo's:
[x] Fix hardcoded absolute program location problem. Program needs the absolute location where its stored on the filesystem, not where its run from!
[x] Check that we keep the copy of the config (because of restore) after program has run --> When restoring we backup to configbackup\programname\temp\. When restoring we copy contents of temp to configbackup\programname\recyclebin.
[x] Cleanup recyclebin after x
[ ] Add cleanup to sync and restore parts.
[ ] Format Help message.
[ ] License stuff. Include license in installer. Display license in help message.
[ ] Add more verbose messages.
[ ] Support pointing to existing Archive during installation (Copy existing Archive to installation directory).
[ ] Support CMake
-------------------------------------------------

-------------------------------------------------
# Reevaluate:

// TODO: Make database path copy function self referencing with error message parameter. Use only this function wherever you copy database paths
-------------------------------------------------

-------------------------------------------------
# Feature requests:

- Implement incremental config saves, (storing modified files in vector)
- Write the integer map in database.hpp serialized to a binary file. 
- Let user change number of saves to keep
- Let user delete saves
- Option to change default behavior of restore param (Disable auto default to newest save, instead show save dates for a program. Let user select the date that he wants to restore)
- Option to identify specific save date with an alias (Like '12.November')
-------------------------------------------------


-------------------------------------------------
# Trashcan:

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
-------------------------------------------------

-------------------------------------------------
# Parking Lot
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
-------------------------------------------------