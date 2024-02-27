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
        
    + restore/rollback [program name] [date to restore]--default:newest.
        - Rollback a programs config by replacing it with a save from the archive.
        - defaults to most recent save.

    + show [program name]

    + status

    + settings
        - Scheduled task On/Off 
            - No intervall provided: default:xxx
        - Change Intervall of scheduled task
        
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
[ ] Cleanup recyclebin after x 
-------------------------------------------------

-------------------------------------------------
# Reevaluate:

// TODO: Make database path copy function self referencing with error message parameter. Use only this function wherever you copy database paths
-------------------------------------------------

-------------------------------------------------
# Feature requests:

- Implement incremental config saves, (storing modified files in vector)
- Write the integer map in database.hpp serialized to a binary file. 
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