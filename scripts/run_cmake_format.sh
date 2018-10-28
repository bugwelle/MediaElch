#!/usr/bin/env bash

set -e

SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"

cd "${SCRIPT_PATH}/.."

echo "Format all CMakeLists.txt using cmake-format"

cmake-format -c .cmake-format -i CMakeLists.txt
