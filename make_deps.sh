#!/bin/bash -ex

readonly CMAKE_CXX_STANDARD=17

readonly ROOT=$(cd $(dirname $0); pwd)

if [[ -n "$1" ]]; then
    depsdir="$1"
else
    depsdir="$ROOT/d"
fi

mkdir -p "$depsdir"
depsdir="$(cd "$depsdir"; pwd)"

echo "Dependency's dir: $depsdir"

llvmdir=$depsdir/llvm
if [[ ! -d "$llvmdir" ]]; then
    git clone "https://github.com/llvm-mirror/llvm.git" "$llvmdir"
else
    cd "$llvmdir"
    git pull --ff-only
    cd -
fi

cmake "-H$llvmdir" "-B${llvmdir}/b" \
    -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD \
    -DCMAKE_INSTALL_PREFIX=$ROOT/i \
    -DCMAKE_BUILD_TYPE=Release

MAKEFLAGS=-j8 cmake --build "${llvmdir}/b" --config Release --target install


