echo OFF

REM Windows batch file to create import libraries for linking ION DLLs in a 
REM Windows development environment such as Visual Studio.  The batch file
REM should be executed at a Visual Studio command prompt after a successful ION
REM build using mingw.
REM 
REM    Example Usage:
REM         winimplib.bat i386
REM         winimplib.bat x64

IF [%1]==[] (
    echo Target must be supplied.
    echo Usage:    winimplib.bat target
    echo Example:  winimplib.bat x86
    goto :eof
)

set target=%1

IF NOT EXIST winimplib mkdir winimplib

cd ams/i86-mingw
lib /machine:%target% /def:libams.def
copy *.lib ..\..\winimplib
cd ../..

cd bp/i86-mingw
lib /machine:%target% /def:libbp.def
REM lib /machine:%target% /def:libbssfw.def
lib /machine:%target% /def:libcgr.def
lib /machine:%target% /def:libdtn2fw.def
lib /machine:%target% /def:libimcfw.def
lib /machine:%target% /def:libipnfw.def
lib /machine:%target% /def:libtcpcla.def
lib /machine:%target% /def:libstcpcla.def
lib /machine:%target% /def:libudpcla.def
copy *.lib ..\..\winimplib
cd ../..

cd bss/i86-mingw
lib /machine:%target% /def:libbss.def
copy *.lib ..\..\winimplib
cd ../..

cd bssp/i86-mingw
lib /machine:%target% /def:libbssp.def
lib /machine:%target% /def:libtcpbsa.def
copy *.lib ..\..\winimplib
cd ../..

cd cfdp/i86-mingw
lib /machine:%target% /def:libcfdp.def
copy *.lib ..\..\winimplib
cd ../..

cd dgr/i86-mingw
lib /machine:%target% /def:libdgr.def
copy *.lib ..\..\winimplib
cd ../..

cd dtpc/i86-mingw
lib /machine:%target% /def:libdtpc.def
copy *.lib ..\..\winimplib
cd ../..

cd ici/i86-mingw
lib /machine:%target% /def:libici.def
copy *.lib ..\..\winimplib
cd ../..

cd ici/libbloom-master
lib /machine:%target% /def:libbloom.def
copy *.lib ..\..\winimplib
cd ../..

cd ltp/i86-mingw
lib /machine:%target% /def:libltp.def
copy *.lib ..\..\winimplib
cd ../..
