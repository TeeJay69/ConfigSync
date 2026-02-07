# v2.9.0
### New features
#### Verify integrity of archive
- Go through all saves and check if they are present (only top level day dir not actual individual saves)
- Users can manually delete saves inside the configarchive (only the day dirs please!!!)
- Shows how much it fixed

### Fixes
- savelimit enforcement doesnt fully work, and we dont delete empty daydirs after removing the timestamp dirs

# v2.8.4
### Hotfix
- Fixes missing return that led to an illegal instruction causing the program to exit prematurely.

# v2.8.3
### Fix
- Syncs all chrome profiles now, not just the default profile

# v2.8.2
### ReadME.md
- Moving to public repo

# v2.8.1
### Fix
- Add hashing fix from v2.7.1 (5c59372c) to all other instances of hashing not just "sync" (status etc.)

# v2.8.0
### New program supported
- Filen
> saves all history, sync directories

# v2.7.3
### Fix
- recurse_remove: Resolved issue where recurse_remove did not delete directories after removing files.
    - Updated the recursive removal function to perform a post-order traversal. This ensures that directories are deleted after all their contents have been removed, preventing leftover empty directories when recursively deleting a path.
    - Adjusted permissions handling to modify permissions before attempting to delete files and directories, ensuring that read-only files are properly removed.
    - Eliminated modification of the directory structure during iteration to prevent undefined behavior.

# v2.7.2
### Fixes
- Delete: Fixes error that led to failures when users tried to delete saves (directory was not found)
    - Explanation: timestamp_to_str_ymd() used localtime, whereas timestamp() and ymd_date() both used UTC. When your local time zone is ahead or behind UTC, especially around midnight, the dates produced by these two functions can differ by one day.
    - Example: if it's 1 AM local time on November 25th but still November 24th in UTC, ymd_date() will return 2023-11-24 (UTC date), while timestamp_to_str_ymd() will return 2023-11-25 (local date).

# v2.7.1
### Fixes
- Sync: Fixes error that would always sync the Elgato-StreamDeck regardless of the sync status
    - Explanation: The get_sha256hash() function is using fopen() which cannot handle non-ASCII filenames. The function threw a Runtime-Error which caused the catch statement to be executed and within that catch, the sync state was set to (equal = 0 (false)). The streamdeck program included non-ASCII named files which caused the issue
    - Fix: Added a new get_sha256hash_cpp function which avoids the fopen() function.
    
# v2.7.0
### New program supported
- Steam
> saves all settings, custom artwork etc.

### Fixes
- Sync: Fixes error that prevented Fusion360 from being synced
    - Explanation: A missing backslash in ""C:\\Users\\" + uName + "\\AppData\\Roaming\\Autodesk\\Neutron Platform\\Options\\" + get_fusion360_dir()" (after "Options") led to a non existing path string.
- Cleanup: Fixes logic error that prevented deletion of the oldest save if the save count is above the savelimit
    - Explanation: The current day was falsely used to create a path that might not exist (currentDay\\oldestTst). Now it correctly converts the oldestTst to a YYYY-MM-DD string format which is used to construct the correct path of the oldest save.

# v2.6.0
### New program supported
- Elgato-StreamDeck
> saves all settings, profiles, plugins

# v2.5.0
### New program supported
- OBS-Studio
> saves all the settings, profiles, scenes#

### Bugs
- Fixed bug that prevented directories from being deleted.

# v2.4.0
### New program supported
- Chatterino
> saves all the settings

# v2.3.0
### New program supported
- foobar2000-v2
> saves all the playlists and files from appdata

# v2.2.1
### Fixes
- Sync/Status: Prevent error when trying to sync a program whose previous save was a forced save
    - Explanation: Forced saves can include files that are not copied by a normal save (e.g. due to file locks by running instances of the target program). When you try to do a normal sync after a forced sync, the program will try to compare the hashes of the files from the previous save with the corresponding "current" files. When we try to hash a locked file, we get a runtime_error.
    - Fix: The program interprets a failed hash attempt as "out-of-sync"
    - Logs: All failed attempts including the filepath are logged.

# v2.2.0
### Support for more programs
- Mp3tag
### Fixes
- Sync: Prevent premature termination due to filesystem permission error. Logging error and continue.
- Help: Prevent logic_error when running `help` without additional arguments.
- List: Display programs in formatted list without additional spaces for index below 10.
### New features
#### Force synchronization operand
- Kill running instances and skip evaluating up-to-date status of previous save.
    - Syntax: `--force, -f`
    - Example: `configsync sync --all --message "test save" --force`

# v2.1.2
### Fixes status/sync option errors
- Filesystem error occurred when trying to hash invalid paths
### Display insync status correctly
- All "insync" programs were falsely labeled as "never synced"

# v2.1.1
### Adds support for new programs
- Fusion360
- Google Chrome
- Mozilla Firefox
- Calibre
- CMD-Macros
- Powershell-Macros
### Detailed program specific info/guides
- New operand: `configsync --help --programs`

# v2.0.1 Hotfix
Patches filesystem exception that occured on most instances

# v2.0.0
- Features a complete redesign of the program
- Better performance and more features

# v1.0.0
- Initial release