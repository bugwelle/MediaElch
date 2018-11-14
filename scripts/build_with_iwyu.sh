#!/usr/bin/env bash

set -e

SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"

pushd "${SCRIPT_PATH}/.." > /dev/null

echo "Build project with include-what-you-use"

mkdir -p ./build
rm -rf build/*
cd build
CC="clang" CXX="clang++" cmake -DCMAKE_CXX_INCLUDE_WHAT_YOU_USE="include-what-you-use;-Xiwyu;--mapping_file=/home/andre/Projects/Private/MediaElch/scripts/qt5_4.impl" ..
make -j2 2>&1 | tee should_remove_headers.txt
if [ -s should_remove_headers.txt ]; then echo "Found unnecessary headers! Use include-what-you-use!"; exit 1; fi

popd > /dev/null
