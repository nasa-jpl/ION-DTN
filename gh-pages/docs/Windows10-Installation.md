# Windows 10 Prototype with Automake Build System

This is a prototype build system for ION on Windows 10. It is not yet thoroughly tested but released for wider experimentation.

## Install MSYS2

- Go to the MSYS2 GitHub repository at [https://github.com/msys2/msys2-installer/releases](https://github.com/msys2/msys2-installer/releases)
- Expand the Assets list in the newest update block
- Locate the installer executable (msys2-x86_64-`<release-date>`.exe).  You can (and should) verify the
  hash and signature for the file.
- Download and run the installer
- The default location for the installation is c:\msys64
- The installer will offer to “run MSYS” by opening a “UCRT64” shell when the installation is finished. Either uncheck the box or close this shell.

Note that the user home directory is located at
“C:\msys64\home” if the default installation location was selected.

## Open a MINGW64 bash shell & Install required packages

- From command prompt, run `C:\msys64\mingw64.exe` - For ION to install correctly, you must be in the correct shell.  Make sure the prompt includes “Mingw64" before proceeding.
- Update base packages of MSYS2 with `pacman -Syu`

  - You may need to restart `mingw64.exe` and run `pacman -Syu` again to complete the update.

### Install packages necessary to build ION

(autotools, mingw64_gcc, etc).

- A script has been provided (`mingw64_packages.sh`) that should install everything needed.
- You can look through the script to see the package list.
- Another "backup" script `mingw_packages_backup.txt` that shows packages specifically for each shell (ex. Mingw_w64 gcc and Msys gcc are different, etc). Install only as needed. We recommend you start with using just the packages in `mingw64_packages.sh` script and try to build ION.

## Build & Install ION

In the top directory of the ION source tree

Run `autoreconf -fi`

Run `./configure` with the options you want

Run `make`

Note: in mingw-w64, the make command is typically installed as `mingw32-make.exe` rather than just `make`, and `MSYS2` doesn’t always create a `make` alias or symlink by default.

Check the directory `/mingw64/bin` to see if you have `make.exe` or `mingw32-make.exe` . If you have `mingw32-make.exe` then create a symbolic link to use `make`.

```
ln -s /mingw64/bin/mingw32-make.exe /mingw64/bin/make.exe
```

Run `make install`

ION is now ready for use; however, for `killm` to work correctly, the execution of scripts must be enabled in Powershell:

- Run `Set-ExecutionPolicy RemoteSigned` in PowerShell in order to enable the running of scripts.
- Note that the “RemoteSigned” policy normally prevents downloaded scripts from running. Currently MSYS2 does not set the ZoneIdentifier meta-date, resulting in all ION files considered to be local.  If that changes in the future, or ION was unpacked in Windows, you can use `unblock-file .\remove_win_shmem.ps1` in PowerShell to remove the ZoneIdentifier.
