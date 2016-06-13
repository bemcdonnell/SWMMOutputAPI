echo off
set PATH=%PATH%;C:\MinGWi686Posix\mingw32\bin

gcc.exe -c -D_FILE_OFFSET_BITS=64 outputAPI.c datetime.c 
gcc.exe -shared -D_FILE_OFFSET_BITS=64 -O3 -o C:\PROJECTCODE\SWMMOutputAPI\data\outputAPI_win.dll outputAPI.o datetime.o 
