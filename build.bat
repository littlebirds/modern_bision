call "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

cmake -G Ninja -S. -Bbuild
cmake --build build
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Compilation failed with error code %ERRORLEVEL%
    echo [SKIPPED] Skipping unit tests due to compilation failure
    exit /b %ERRORLEVEL%
)
ctest --test-dir build -V
