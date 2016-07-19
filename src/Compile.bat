echo off
set PATH=%PATH%;C:\MinGW\bin

gcc.exe -c -Wall outputAPI.c datetime.c 
gcc.exe -mwindows -o C:\PROJECTCODE\SWMMOutputAPI\data\outputAPI_win3.dll -shared outputAPI.o datetime.o

del /S *.o