 - To make restoring the configuration/settings of programs easier when reinstalling them or when moving to a new system etc
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
    Plan:
        - Per program stores config in extra directory by date.
        - Checks if config changed.
            - If it did, keeps the previous config folder and creates a new one with current date
            - If didnt change, rename the folder to current date
        - Keeps x number of previous config folders when x = currentNumber, delete oldest folder
        - config file lists programs for which a config backup will be created // needed?
        - programs.hpp class for program specific paths/info
        - Implement incremental config saves, (storing modified files in vector)


restore config:
    ~~- Check if current config exists~~
    - Create backup of current config:
        - move any existing items inside the ConfigBackup\\program\\temp directory to ConfigBackup\\program\\RecycleBin
        - copy C:\\program\\configPath to programs ConfigBackup\\program\\temp directory, name with UUID or pseudo random value.
    - Copy each directory in ConfigArchive\\program\\newestDate\\ to its corresponding C:\\program\\configPath directory. Overwrite existing files.
    - On Error, 
        - copy each directory in ConfigBackup\\program\\temp to its corresponfing C:\\program\\configPath (using UUID). Overwrite existing files.
        - move backup inside the ConfigBackup\\program\\temp directory to ConfigBackup\\program\\RecycleBin
    - On Success: 
        - move backup inside the ConfigBackup\\program\\temp directory to ConfigBackup\\program\\RecycleBin
        - If RecycleBin has more than x number of items, delete x number of oldest items in RecycleBin directory.
        - Inform user that config was restored successfully


// TODO: Make database path copy function self referencing with error message parameter. Use only this function wherever you copy database paths
~~// TODO: Fix hardcoded absolute program location problem. Program needs the absolute location where its stored on the filesystem, not where its run from!~~