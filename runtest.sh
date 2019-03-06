#!/usr/bin/env bash

set -x

EXT=$1
TEST=$2

if [ ${EXT} = "c" ]; then
    COMPILER="clang"
elif [ ${EXT} = "cpp" ]; then
    COMPILER="clang++"
fi

if [[ "${OSTYPE}" == "darwin"* ]]; then
    export DYLD_INSERT_LIBRARIES=build/mallocazam/libmallocazam_catch_dlclose.dylib
    export DYLD_FORCE_FLAT_NAMESPACE=1
else
    export LD_PRELOAD=build/mallocazam/libmallocazam_catch_dlclose.so
fi

${COMPILER} -S -emit-llvm -Xclang -load -Xclang build/mallocazam/libmallocazam.so test/${TEST}.${EXT} -o /dev/stdout
