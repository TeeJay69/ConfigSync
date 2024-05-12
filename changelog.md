# v2.2.0
### Support for more programs
- Mp3tag
### Fixes
- Sync: Prevent premature termination due to filesystem permission error. Logging error and continue.
- Help: Prevent logic_error when running `help` without additional arguments.
- List: Display programs in formatted list without additional spaces for index below 10.
### New features
#### Force synchronization operand
- Kill running instances and skip evaluation if previous save is already up-to-date.
    - Syntax: `--force, -f`

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