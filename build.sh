#!/bin/bash
set -e

BUILD_DIR="build"
CMAKE_ARGS=(-S. -B"${BUILD_DIR}")

# macOS: use Homebrew bison if available to avoid system bison 2.3
if [[ "$OSTYPE" == "darwin"* ]]; then
    BISON_PATH=""
    for path in \
        /opt/homebrew/opt/bison/bin/bison \
        /opt/homebrew/Cellar/bison/*/bin/bison \
        /usr/local/opt/bison/bin/bison \
        /usr/local/Cellar/bison/*/bin/bison; do
        if [[ -f "$path" ]]; then
            BISON_PATH="$path"
            break
        fi
    done

    if [[ -n "$BISON_PATH" ]]; then
        echo "[INFO] macOS detected. Using Homebrew Bison: ${BISON_PATH}"
        CMAKE_ARGS+=("-DBISON_EXECUTABLE=${BISON_PATH}")
    else
        echo "[WARN] macOS detected but no Homebrew Bison found. CMake will use system bison."
    fi
fi

echo "[INFO] Cleaning previous CMake cache..."
rm -rf "${BUILD_DIR}/CMakeCache.txt" "${BUILD_DIR}/CMakeFiles"

echo "[INFO] Configuring..."
cmake "${CMAKE_ARGS[@]}"

echo "[INFO] Building..."
cmake --build "${BUILD_DIR}"

echo "[INFO] Running unit tests..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "[SUCCESS] Build and tests completed."
