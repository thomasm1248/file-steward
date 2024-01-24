
# File Steward

File Steward is a simple command-line tool for automating repetitive file management operations. When it runs, it reads instructions from a specified config file. It will then perform a variety of simple actions as described in the config file.

#### Syntax

```
./<executable> <path to config file>
```

## Possible actions

### Remove expired files from specified folder

A folder can be marked as a temporary folder, and file-steward will delete files older than a specified number of days.

#### Syntax

```
tempFolder <number of days> <absolute path to folder>
```

#### Example

The following instruction tells file-steward to delete downloaded files that are older than 20 days:

```
tempFolder 20 /home/username/Downloads
```

### Zip modified subfolders in a specified folder

A folder can be marked as an archive folder, and file-steward will look inside for folders that have been modified since the last run. If it finds modified folders, they will be zipped. It's then your job to move those zipped folders to a backup location.

#### Syntax

```
archiveFolder <absolute path to folder>
```

#### Example

The following instruction tells file-steward to check if any of the folders inside of Archive/ have been modified. If any of them have, zip them so you can upload the zip files to cloud storage for backup:
```
archiveFolder /home/username/Archive
```

### Move files from one folder to another

A source folder and a destination folder are chosen, and all the files in the source folder are moved to the destination folder.

Note: This action is currently under development, and is currently unable to move files to a destination folder path that contains spaces.

#### Syntax

```
moveFiles <absolute path to destination folder> <absolute path to source folder>
```

#### Example

Move all the files from the Downloads folder to the OldDownloads folder.

```
moveFiles /home/username/Downloads /home/username/OldDownloads
```
