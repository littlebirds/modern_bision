@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

cmake -DCMAKE_BUILD_TYPE=Debug -S. -Bbuild > build_log.txt 2>&1
cmake --build build >> build_log.txt 2>&1
type build_log.txt
