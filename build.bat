call "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

cmake -G Ninja -S. -Bbuild
cmake --build build
ctest --test-dir build --output-on-failure
