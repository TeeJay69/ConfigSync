# **ConfigSync** 

is a powerful configuration backup, restoration, and synchronization tool for Windows, written in C++. It allows you to seamlessly manage configuration files for multiple programs, ensuring consistency across various devices and instances.

> **Note**: This project is a passion project by [T. Jason Weber]. For any feedback or contributions, please open an issue or contact me directly.

## Table of Contents

1. [Features](#features)  
2. [Why ConfigSync?](#why-configsync)  
3. [How It Works](#how-it-works)  
4. [Installation](#installation)  
   - [Prerequisites](#prerequisites)  
   - [Build Steps](#build-steps)  
5. [Usage](#usage)  
   - [Basic Commands](#basic-commands)  
   - [Advanced Commands](#advanced-commands)  
6. [Configuration](#configuration)  
   - [Settings Overview](#settings-overview)  
   - [JSON Configuration File](#json-configuration-file)  
7. [Logs](#logs)  
8. [Supported Programs](#supported-programs)  
9. [Troubleshooting](#troubleshooting)  
10. [License](#license)  
11. [Contributing](#contributing)  
12. [Acknowledgements](#acknowledgements)  

---

## Features

- **Backup and Restore**  
  Save and restore configuration settings for over 20 supported programs. You can specify a particular date (timestamp) or revert to the latest save.  

- **Automatic Version Control**  
  ConfigSync maintains distinct backups by timestamps, making it easy to return to any previous state.  

- **Scheduled Tasks**  
  Automate backups with configurable scheduling. Backups can run **daily** or **hourly** without user intervention.  

- **Safety Mechanisms**  
  Includes features like _pre-restore backups_ to prevent accidental data loss should something go wrong.  

- **Detailed Logs**  
  Each sync or restore operation is logged with timestamps, messages, and error details.  

- **Rich Configuration**  
  Fine-tune how many backups to keep, scheduling frequencies, editor preferences, and more.  

## Why ConfigSync?

1. **Frictionless Multi-Device Workflow**  
   Move from one machine to another without having to reconfigure your favorite programs each time.  

2. **Hash-Based Checks**  
   ConfigSync doesn’t rewrite or store duplicate data when nothing has changed, saving time and space.  

3. **Safe Recovery**  
   If a restore goes awry, you can revert back to a prior state quickly using the _pre-restore backup_.  

4. **Granular Control**  
   Only want to sync a particular program’s settings? Or do you want to sync everything at once? ConfigSync has you covered with simple CLI commands.  

---

## How It Works

1. **Backup Configurations**  
   - ConfigSync *first* checks if a program’s existing configurations match what’s in the current archive by computing file hashes.  
   - If it detects any differences (i.e., the program is “out of sync”), **it creates a brand-new “full save”** that includes *all* the program’s configuration files.  
   - By doing so, you always have multiple complete snapshots for each date. If ConfigSync were to only copy or overwrite changed files, it could complicate the archive structure and make it harder to revert to distinct states in time.  

2. **Restore Configurations**  
   - Easily revert to a specific timestamp using `--date [YYYY-MM-DD]`.  
   - ConfigSync also creates a *pre-restore backup* before overwriting files to safeguard against partial or failed restores.

3. **Synchronization**  
   - Keep your program configurations in sync across multiple devices by running or scheduling sync commands.  
   - Each “out of sync” detection triggers a new, fully contained save, ensuring a complete record of state changes over time.

> **Note**: Although ConfigSync compares file hashes for checking sync status, it doesn’t rely on partial or differential backups. Instead, it maintains separate, **complete** snapshots for every save date. This approach simplifies restoration and preserves a clear history of each program’s configuration.

---

## Installation

### Prerequisites

- **Windows 10 (or later)**  
- **C++ Compiler** (e.g., G++ (e.g. ucrt64), Visual Studio, MSVC, Clang)  
- **Boost Libraries** (optional headers like `<boost/uuid>`, `<boost/property_tree>`, etc. as used in `main.cpp`)  
- **C++ Redistributables** (if required by your system)

> **Optional**: For editing the `settings.json` directly, having an editor like **Notepad**, **Vim**, or **VSCode** is helpful.

### Build Steps

1. **Clone the Repository**  
   ```bash
   git clone https://github.com/YourUsername/ConfigSync.git
   cd ConfigSync
   ```

2. **Build the Project**  
   Use your preferred method, such as Visual Studio, CMake, or a command-line compiler:
   ```bash
   # Example with Visual Studio Developer Command Prompt
   cl /EHsc main.cpp /I C:\Path\To\boost_<version> /FeConfigSync.exe
   ```

   Or using MinGW g++:
   ```bash
   g++ -std=c++17 -I C:/Path/To/boost_<version> -o ConfigSync.exe main.cpp ConfigSync.hpp
   ```

3. **Run the Executable**  
   ```bash
   ConfigSync.exe
   ```
   If you see a copyright notice, everything was built successfully!

---

Below is an **optional** section you can add under **Installation** or in a dedicated **Compilation** section to clarify your use of the ucrt64 toolchain, the differences from older MinGW, and how people can use your custom building tool or `.versiontool` file.

---

## Usage

All commands follow the pattern:
```bash
configsync [COMMAND] [OPTIONS] [--flags]
```

### Basic Commands

- **Synchronize (Backup) a Specific Program**  
  ```bash
  configsync sync [program-name]
  ```
  *Example:*
  ```bash
  configsync sync notepad++ --message "Before plugin update"
  ```

- **Restore Configurations**  
  ```bash
  configsync restore [program-name] [--date YYYY-MM-DD] [--force]
  ```
  - `--force`: Kills the program’s running processes before restoring (if needed).  

- **List Supported Programs**  
  ```bash
  configsync list
  ```

- **Check Synchronization Status**  
  ```bash
  configsync status        # Checks all programs
  configsync status paint  # Checks only Paint config
  ```

- **View Saved Dates for a Program**  
  ```bash
  configsync show [program-name]
  ```
  This displays timestamps and optional messages for each save.

### Advanced Commands

- **Undo (Restore the “Pre-Restore-Backup”)**  
  ```bash
  configsync undo [program-name] --restore [--date YYYY-MM-DD] [--force]
  ```
  Roll back a restore operation by applying the pre-restore backup if something went wrong.  

- **Delete Oldest Saves**  
  ```bash
  configsync delete [program-name] --number X
  ```
  This removes the X oldest saves to free up space if you exceed your limit.  

- **Settings Management**  
  ```bash
  # View current settings
  configsync settings
  
  # Reset a specific setting (example: taskfrequency)
  configsync settings --reset taskfrequency
  
  # Open raw JSON file in your preferred editor
  configsync settings --json
  ```
  The `settings` command is extensive; use `help` for details or view the next section below.

For more detailed command usage, run:
```bash
configsync --help
```

---

## Configuration

### Settings Overview

In `settings.json`, you will find (at minimum) the following keys:

- **`savelimit`**: Maximum number of saves to keep per program.  
- **`pre-restore-limit`**: Number of pre-restore backups to maintain.  
- **`task`**: Enable or disable scheduled tasks (`true` or `false`).  
- **`taskfrequency`**: Frequency for scheduled tasks (e.g., `"daily,1"`).  
- **`editor`**: Preferred text editor for opening `settings.json` (e.g., `"vscode"`, `"vim"`, `"notepad"`).

### JSON Configuration File

The `settings.json` is located in the same folder as `ConfigSync.exe`. Example snippet:
```json
{
    "settingsID": 1,
    "savelimit": 60,
    "pre-restore-limit": 60,
    "task": false,
    "taskfrequency": "daily,1",
    "editor": "vscode"
}
```
> **Warning**: Modifying this file incorrectly may cause parse errors. Always back it up or use `configsync settings` to make changes safely.

---

## Logs

ConfigSync produces detailed logs in the `logs` directory, found alongside the executable. Each log is named by timestamp:
```
logs
 ┣ 2024-12-21_14-23-05.log
 ┣ 2024-12-21_15-32-01.log
 ┗ ...
```
Logs contain:
- **Timestamps** for each action (sync, restore, error).
- **Error Messages** in case of runtime or filesystem errors.
- **Summary** of operations performed.

---

## Supported Programs

ConfigSync currently supports over 20 programs, ranging from text editors to IDEs. You can see the full list by running:
```bash
configsync list
```

### List of programs (might lag a few commits behind)

>New programs can be added by editing the internal `Programs::Mgm` mappings (found in the source code).

1.  Calibre
2.  Chatterino
3.  CMD-Macros
4.  Deemix
5.  Elgato-StreamDeck
6.  Filen
7.  foobar2000-v2
8.  Fusion360
9.  Google.Chrome
10. Jackett
11. JDownloader
12. Microsoft.PowerToys
13. Microsoft.VisualStudioCode
14. Mozilla.Firefox
15. Mp3tag
16. OBS-Studio
17. Powershell-Macros
18. Prowlarr
19. qBittorrent
20. Steam
21. Ultimaker.Cura
---

## Troubleshooting

1. **“No saves found” error**  
   - Ensure you have synchronized (backup) at least once for the target program.  
   - Check if you’re referencing the correct program name (use `configsync list` to confirm).  

2. **Scheduled Task Doesn’t Run**  
   - Confirm you have enabled tasks: `configsync settings.task true`.  
   - Verify the task was created by running `configsync check --task`.  
   - Windows Defender might flag tasks incorrectly; you may need to whitelist them.  

3. **Restore Fails or Partially Works**  
   - Run restore again with `--force` to close any running instances that might lock files.  
   - Check logs in the `logs` folder for any errors.  

4. **Incorrect Paths or Hard-Coded Directories**  
   - Certain programs (like Calibre) may store absolute paths in config. Ensure both machines have the same directory structure if you want perfect interoperability.  

5. **JSON Parse Errors**  
   - If your `settings.json` is corrupt, run `configsync settings --reset --all` to revert to defaults.

---
## Additional Info
## 
## Compilation Notes

ConfigSync can be built on Windows using various toolchains. Below is guidance if you want to use the **ucrt64** toolchain (recommended) or if you prefer the older MinGW-based approach.

### Why Use ucrt64?

1. **ucrt64** is a newer MinGW-w64 environment that uses the **Universal C Runtime (UCRT)** instead of the older **MSVCRT**.  
2. Compiled applications often require fewer external DLLs, and the runtime is generally more up to date.  
3. If you rely on UNIX-like commands (e.g., `touch`, `cp`), you may need to install additional MSYS2 tools:
   ```bash
   pacman -S msys/coreutils
   ```
4. Installation of the main toolchain:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-toolchain
   ```

### Differences from Older MinGW (MSVCRT)

- **MinGW (MSVCRT)**  
  - Considered “older” but was widely used for a long time.  
  - Often requires `msys-2.0.dll` at runtime.  
  - May be deprecated or EOL soon in some contexts.  

- **ucrt64** (MinGW-w64 with Universal CRT)  
  - Newer, uses Microsoft’s Universal C Runtime (UCRT).  
  - May need extra packages for Unix-like utilities (e.g., `coreutils`).  
  - Generally preferred moving forward.

### Using the `.versiontool` File

You’ll notice a `.versiontool` file in the repository, which **automates** building by specifying most compile flags and libraries. You can either:

1. **Use Your Custom Building Tool**  
   - If you have a custom C++ build tool that reads `.versiontool`, simply run it according to your documentation. This tool will parse the file and construct the right `g++` command automatically.

2. **Manually Copy the Compile Command**  
   - Open `.versiontool` and look for a line similar to:  
     ```txt
     g++ main.cpp -o ConfigSync.exe -std=c++23 -static-libgcc -static-libstdc++ -Wl,-Bstatic \
         -lstdc++ -lpthread -lbcrypt -lssl -lcrypto -lboost_filesystem-mt -lz -O3
     ```
   - Adjust the paths for **Boost** and any libraries needed on your system, then run it in a **MinGW-w64 UCRT64** shell.

> **Note**: Some libraries, like `openssl`, `crypto`, or `boost_filesystem-mt`, may need to be installed. Use `pacman -S <package>` (in MSYS2) to ensure these dependencies are present.

### Example ucrt64 Build Command

From an MSYS2 MinGW-ucrt64 shell, you might do something like:
```bash
# Install necessary packages
pacman -S --needed mingw-w64-ucrt-x86_64-boost mingw-w64-ucrt-x86_64-openssl msys/coreutils

# Navigate to the project folder
cd /c/path/to/ConfigSync

# Run the g++ command (either from .versiontool or manually typed)
g++ main.cpp -o ConfigSync.exe -std=c++23 -static-libgcc -static-libstdc++ \
    -Wl,-Bstatic -lstdc++ -lpthread -lbcrypt -lssl -lcrypto -lboost_filesystem-mt -lz -O3
```

After successful compilation, you can run:
```bash
./ConfigSync.exe
```
(or `ConfigSync.exe` from Windows Explorer).

---


## Syncing CMD & PowerShell Macros

ConfigSync also supports syncing macros or aliases for the Windows Command Prompt (CMD) and PowerShell:

- **CMD-Macros**  
  By default, CMD macros (or aliases) are stored in a `.doskey` file. To ensure these macros are loaded whenever you open a CMD window, you can add a registry entry:
  ```bash
  reg add "HKCU\Software\Microsoft\Command Processor" /v Autorun /d "doskey /macrofile=c:\data\<username>\software\settings\cmd\profile\macros.doskey" /f
  ```
  You can verify the entry by running:
  ```bash
  reg query "HKCU\Software\Microsoft\Command Processor" /v Autorun
  ```
  With ConfigSync, you can back up and restore this `.doskey` file and keep the macros consistent across machines.

- **PowerShell-Macros**  
  PowerShell aliases (or functions) are often stored in a user profile script (e.g., `$PROFILE`). For easy synchronization and backup:
  1. Create a file at `C:\Data\<username>\Software\Settings\Powershell\profile\Microsoft.PowerShell_profile.ps1`.
  2. Copy the contents of your existing `$PROFILE` into that file.
  3. Replace the original `$PROFILE` content with:
     ```powershell
     $profile = "C:\Data\<username>\Software\Settings\Powershell\profile\Microsoft.PowerShell_profile.ps1"
     . $profile
     ```
  This way, ConfigSync can back up the file `Microsoft.PowerShell_profile.ps1`, letting you restore or share your custom aliases on other systems.

> **Tip**: ConfigSync sees these macros as just another set of configuration files to back up and restore. Simply add them to your list of supported “programs” (e.g., `CMD-Macros`, `Powershell-Macros`), and ConfigSync will track their changes, store them in the archive, and allow easy rollback or synchronization across devices.

---

## License

ConfigSync is **proprietary software**. Use and modification of the code are subject to the terms in the [Copyright Disclaimer](./Copyright-Disclaimer.md).

---

## Contributing

Your feedback and improvements are welcome! However, as this is a **proprietary** project, please reach out to [T. Jason Weber] for permission if you wish to submit larger changes or add new features.  

**Ways to contribute**:  
- **Bug Reports**: Create an issue on GitHub with detailed logs.  
- **Feature Requests**: Open a discussion or issue describing the feature.  
- **New Program Support**: Submit suggestions for new program configuration paths or open a PR once approved.

---

## Acknowledgements

- **Boost Libraries** – For property trees, UUIDs, and filesystem support.  
- **Open-Source Community** – Inspiration for many patterns and solutions used here.  
- Special thanks to everyone who has tested, reported bugs, or suggested features.

---

**Happy Syncing!**  
_If you find ConfigSync helpful, consider sharing it with others who might also benefit._

# **FAQ**
---

## Moving to a New Machine

Because **ConfigSync** doesn’t rely on cloud storage, you’ll need to **manually** copy the tool and its data from one machine to another. Fortunately, this is straightforward:

1. **Locate Your Current Installation**  
   - By default, if you used the `ConfigSync-Setup.exe` installer, the program’s files reside in:
     ```
     %LOCALAPPDATA%\ConfigSync
     ```
   - If you built or placed `ConfigSync.exe` elsewhere, look for the folder containing both `ConfigSync.exe` and subfolders like `ConfigArchive`, `PreRestoreBackups`, and an `objects` folder with the `Saves.bin` file.

2. **Copy the Entire Folder**  
   - Copy this **entire** directory (including `ConfigSync.exe`, `ConfigArchive`, `PreRestoreBackups`, `objects`, etc.) onto your new system.  
   - You can place it in any convenient directory or drive (e.g., `D:\Utilities\ConfigSync`).  

3. **Run `ConfigSync.exe` on the New Machine**  
   - Upon launch, ConfigSync automatically detects the current drive letter, the active username, and the parent folder (`pLocat`) of `ConfigSync.exe`.  
   - It compares these new values against what was stored in your existing backups.  
   - If a mismatch is found (e.g., different drive letter or username), ConfigSync **dynamically updates** the saved paths for that environment, ensuring consistency with your new machine’s file paths.

### How Are Paths Managed Internally?

- **Absolute Paths in the Archive**  
  Each configuration file’s path (e.g., `C:\Users\OldUser\AppData\Roaming\...`) is stored in the internal saves.  
- **Dynamic Updates on Different Machines**  
  In the source code, you’ll see checks like `cmp_root` and `replace_root` or `cmp_uname` and `replace_uname`. These functions detect when the drive letter (root) or the Windows username is different and **automatically** update all archived paths to match the new environment.  
- **Relative Directories Around `ConfigSync.exe`**  
  The application also constructs folders like `ConfigArchive` or `objects\Saves.bin` relative to wherever `ConfigSync.exe` is found, thanks to:
  ```cpp
  pLocatFull = boost::dll::program_location().string();
  pLocat     = boost::dll::program_location().parent_path().string();
  archiveDir = pLocat + "\\ConfigArchive";
  ...
  ```
  This allows you to run `ConfigSync.exe` **from any folder** without needing a fixed path.

### Caveats

- **Program-Specific Hard-Coded Paths**  
  Some programs (e.g., Calibre, certain game launchers) store absolute paths within their own config files, which may not dynamically adapt if you install them to a completely different directory on the new machine. ConfigSync will still restore these files, but the program itself might expect them to exist in the original location.  
- **Username Changes**  
  ConfigSync does its best to rewrite stored paths for the new username. However, if a program references your username in places beyond simple file paths (e.g., deep within the program’s own logic), further manual adjustments may be needed.

> **In most cases**, you can freely move `ConfigSync.exe` to a new location or drive and copy its entire folder to another machine. Upon the first run, ConfigSync’s internal path-adjustment logic ensures your backups remain valid and usable in the new environment.

---