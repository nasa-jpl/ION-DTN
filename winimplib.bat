REM Windows batch file to create import libraries for linking ION DLLs in a 
REM Windows development environment such as Visual Studio.  The batch file
REM should be executed at a Visual Studio command prompt after a successful ION
REM build using mingw.  NOTE: This batch file assumes a 32-bit build.

IF NOT EXIST winimplib mkdir winimplib

cd ams/i86-mingw
lib /machine:i386 /def:libams.def
copy *.lib ..\..\winimplib
cd ../..

cd bp/i86-mingw
lib /machine:i386 /def:libbp.def
REM lib /machine:i386 /def:libbssfw.def
lib /machine:i386 /def:libcgr.def
lib /machine:i386 /def:libdtn2fw.def
lib /machine:i386 /def:libimcfw.def
lib /machine:i386 /def:libipnfw.def
lib /machine:i386 /def:libtcpcla.def
lib /machine:i386 /def:libudpcla.def
copy *.lib ..\..\winimplib
cd ../..

cd bss/i86-mingw
lib /machine:i386 /def:libbss.def
copy *.lib ..\..\winimplib
cd ../..

cd bssp/i86-mingw
lib /machine:i386 /def:libbssp.def
lib /machine:i386 /def:libtcpbsa.def
copy *.lib ..\..\winimplib
cd ../..

cd cfdp/i86-mingw
lib /machine:i386 /def:libcfdp.def
copy *.lib ..\..\winimplib
cd ../..

cd dgr/i86-mingw
lib /machine:i386 /def:libdgr.def
copy *.lib ..\..\winimplib
cd ../..

cd dtpc/i86-mingw
lib /machine:i386 /def:libdtpc.def
copy *.lib ..\..\winimplib
cd ../..

cd ici/i86-mingw
lib /machine:i386 /def:libici.def
copy *.lib ..\..\winimplib
cd ../..

cd ltp/i86-mingw
lib /machine:i386 /def:libltp.def
copy *.lib ..\..\winimplib
cd ../..
