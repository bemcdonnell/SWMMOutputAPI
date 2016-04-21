echo off
set PATH=%PATH%;C:\MinGW\bin

C:\MinGW\bin\gcc.exe -c outputAPI.c datetime.c
C:\MinGW\bin\gcc.exe -shared -o C:\PROJECTCODE\SWMMOutputAPI\data\outputAPI_win.dll outputAPI.o datetime.o
