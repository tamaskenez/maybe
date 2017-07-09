#!/bin/bash -ex

readonly CMAKE_CXX_STANDARD=17

readonly ROOT=$(cd $(dirname $0); pwd)

if [[ -n "$1" ]]; then
    deps="$1"
else
    deps="$ROOT/d"
fi

mkdir -p "$deps"
deps="$(cd "$deps"; pwd)"

echo "Dependency's dir: $deps"

if [[ ! -d "$deps/llvm" ]]; then
    git clone "https://github.com/llvm-mirror/llvm.git" "$deps/llvm"
else
    cd "$deps/llvm"
    # git pull --ff-only
    cd -
fi

if [[ ! -d "$deps/nowide-standalone" ]]; then
    git clone "https://github.com/tamaskenez/nowide-standalone.git" "$deps/nowide-standalone"
else
    cd "$deps/nowide-standalone"
    git pull --ff-only
    cd -
fi

if [[ ! -d "$deps/microlib" ]]; then
    git clone "https://github.com/tamaskenez/microlib.git" "$deps/microlib"
else
    cd "$deps/microlib"
    git pull --ff-only
    cd -
fi

export MAKEFLAGS=-j8

cmake "-H$deps/llvm" "-B$deps/llvm/b" \
    -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD \
    -DCMAKE_INSTALL_PREFIX=$ROOT/i \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$deps/llvm/b" --config Release --target install


cmake "-H$deps/nowide-standalone" "-B$deps/nowide-standalone/b" \
    -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD \
    -DCMAKE_INSTALL_PREFIX=$ROOT/i \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$deps/nowide-standalone/b" --config Release --target install

cmake "-H$deps/microlib" "-B$deps/microlib/b" \
    -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD \
    -DCMAKE_INSTALL_PREFIX=$ROOT/i \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$deps/microlib/b" --config Release --target install
