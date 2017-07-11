#!/bin/bash -e

readonly HAS_XCODE=$(cmake --help | grep Xcode | wc -l) # 1 if has else 0
readonly HAS_VS=$(cmake --help | grep "Visual Studio" | wc -l) # 1 if has else 0
readonly CMAKE_CXX_STANDARD=17
readonly BUILD_THREADS=8
readonly ROOT=$(cd $(dirname $0); pwd)

if [[ $HAS_XCODE == "1" ]]; then
    readonly IDE_GENERATOR=-GXcode
    readonly HAS_IDE_GENERATOR=1
elif [[ $HAS_VS == "1" ]]; then
    readonly IDE_GENERATOR= # use the default VS generator
    readonly HAS_IDE_GENERATOR=1
else
    readonly IDE_GENERATOR=
    readonly HAS_IDE_GENERATOR=0
fi

if [[ -n "$1" ]]; then
    deps="$1"
else
    deps="$ROOT/d"
fi

mkdir -p "$deps"
deps="$(cd "$deps"; pwd)"

echo "Dependency's dir: $deps"

gitit () {
    url="$1"
    name="$2"
    dir="$deps/$name"
    if [[ ! -d "$dir" ]]; then
        echo -e "\n-- Cloning [$name]: git clone $url $dir"
        git clone "$url" "$dir"
    else
        cd "$dir"
        echo -e "\n-- Updating [$name]: cd $dir && git pull --ff-only"
        git pull --ff-only
        cd -
    fi
}

cmakeit () {
    name=$1
    shift
    use_ide_generator=0
    config_build_type=
    if [[ $1 == "--try-use-ide" ]]; then
        if [[ $HAS_IDE_GENERATOR == "1" ]]; then
            use_ide_generator=1
            config_build_type=-DCMAKE_BUILD_TYPE=Release
        fi
    fi
    dir="$deps/$name"
    echo -e "\n-- Configuring [$name] with cmake"
    (set -x; \
        cmake "-H$dir" "-B$dir/b" \
        -DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD \
        -DCMAKE_INSTALL_PREFIX=$ROOT/i \
        $config_build_type \
        $IDE_GENERATOR)
    echo -e "\n-- Building [$name] with cmake"
    if [[ $use_ide_generator == "1" ]]; then
        (set -x; \
            MAKEFLAGS=-j$BUILD_THREADS cmake \
                --build "$dir/b" \
                --config Debug \
                --target install)
        (set -x; \
            MAKEFLAGS=-j$BUILD_THREADS cmake \
                --build "$dir/b" \
                --config Release \
                --target install)
    else
        (set -x; \
            MAKEFLAGS=-j$BUILD_THREADS cmake \
                --build "$dir/b" \
                --config Release \
                --target install)
    fi
}

gitit "https://github.com/llvm-mirror/llvm.git" "llvm"
gitit "https://github.com/tamaskenez/nowide-standalone.git" "nowide-standalone"
gitit "https://github.com/fmtlib/fmt.git" "fmt"
gitit "https://github.com/tamaskenez/microlib.git" "microlib"
gitit "https://github.com/mpark/variant.git" "variant"

# cmakeit "llvm"
cmakeit "nowide-standalone"
cmakeit "fmt"
cmakeit "variant"
cmakeit "microlib" --try-use-ide

